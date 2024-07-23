#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include "editor.h"

static str_t commandline_prompt = STR(">> ");

static void pane_new(struct pane *p) {
	str_t foo = STR("WOOOOHOOO hello there this is some text\nand it is in a pane :-)\nthis line is really jfaalew jfoiewaj oifjaweoijflaonge ong f jaewjf oiawejf oiajewoif jaoiewjf oiajewf043aj9f43aj09j 09j09");
	p->cursor_line = str_to_buflines(foo);
	p->cursor_line_idx = 0;
	p->show_line_nums = 1;
	p->stored_cursor_x = 0;
	p->stored_cursor_y = 0;
}

static void pane_free(struct pane *p) {
	struct bufline *first_line;
	for (first_line = p->cursor_line; first_line && first_line->prev; first_line = first_line->prev)
		;
	free_bufline_list(first_line);
}

void editor_new(struct editor *e) {
	e->mode = MODE_NORMAL;
	e->commandline = string_new();
	e->errormsg = string_new();
	e->should_exit = 0;
	pane_new(&e->foo);
}

void editor_free(struct editor *e) {
	pane_free(&e->foo);
	string_free(e->commandline);
	string_free(e->errormsg);
}

static void render_flowed_text(struct framebuf *fb, struct rect area, str_t text, struct style sty) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	size_t line_start_idx = 0;
	for (int line_y = 0; line_y < area.height; line_y++) {
		struct rect line_area = {
			.x = area.x,
			.y = area.y + line_y,
			.width = area.width,
			.height = 1,
		};
		str_t line = str_slice_idx_to_eol(text, line_start_idx);
		line_start_idx += line.len + 1;
		render_str(fb, line_area, line, sty);
	}
}

static void pane_render(struct pane *p, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	struct rect gutter_area = { .x = area.x, .y = area.y };
	struct rect content_area = area;
	if (p->show_line_nums) {
		gutter_area.height = area.height;
		gutter_area.width = 4;
		content_area.width -= gutter_area.width;
		content_area.x += gutter_area.width;
	}

	struct rect line_area = content_area;
	line_area.height = 1;
	p->stored_cursor_x = line_area.x + p->cursor_line_idx;
	p->stored_cursor_y = line_area.y;

	struct rect line_num_area = gutter_area;
	line_num_area.height = 1;

	size_t cur_line_no = 1;
	for (struct bufline *bl = p->cursor_line; bl != NULL; bl = bl->next) {
		if (line_area.height > content_area.height)
			break;

		char linenum[10];
		sprintf(linenum, "%3zu ", cur_line_no);
		cur_line_no++;

		str_t linenum_str = { .ptr = linenum, .len = strlen(linenum) };
		render_flowed_text(fb, line_num_area, linenum_str, GUTTER_STYLE);
		line_num_area.y += 1;

		render_flowed_text(fb, line_area, string_as_str(bl->string), NORMAL_STYLE);
		line_area.y += 1;
	}
}

static void render_statusline(struct editor *e, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	render_solid_color(fb, area, LIGHTBG_COLOR);

	struct style modestyle;
	str_t modestr;

	switch (e->mode) {
	case MODE_NORMAL:
		modestyle = STATUSLINE_NORMAL_MODE_STYLE;
		modestr = STR(" NORMAL ");
		break;
	case MODE_COMMAND:
		modestyle = STATUSLINE_COMMAND_MODE_STYLE;
		modestr = STR(" COMMAND ");
		break;
	case MODE_INSERT:
		modestyle = STATUSLINE_INSERT_MODE_STYLE;
		modestr = STR(" INSERT ");
		break;
	}

	struct rect mode_area = { .x = area.x, .y = area.y, .width = modestr.len, .height = 1 };
	render_str(fb, mode_area, modestr, modestyle);
}

void editor_render_cursor(struct editor *e, struct rect editor_area) {
	int cursorx;
	int cursory;

	switch (e->mode) {
	case MODE_NORMAL:
		fwrite(BLOCK_CURSOR_ESC, 1, strlen(BLOCK_CURSOR_ESC), stdout);
		cursorx = e->foo.stored_cursor_x;
		cursory = e->foo.stored_cursor_y;
		break;
	case MODE_INSERT:
		fwrite(BAR_CURSOR_ESC, 1, strlen(BAR_CURSOR_ESC), stdout);
		cursorx = e->foo.stored_cursor_x;
		cursory = e->foo.stored_cursor_y;
		break;
	case MODE_COMMAND:
		fwrite(BAR_CURSOR_ESC, 1, strlen(BAR_CURSOR_ESC), stdout);
		cursorx = commandline_prompt.len + e->commandline.len;
		cursory = editor_area.y + editor_area.height;
		break;
	}

	char moveesc[sizeof("\033[XXX;XXXH") - 1] = "";
	sprintf(moveesc, "\033[%d;%dH", cursory + 1, cursorx + 1);
	fwrite(moveesc, 1, strlen(moveesc), stdout);
}

void editor_render(struct editor *e, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	int commandline_line_used = e->mode == MODE_COMMAND || e->errormsg.len > 0;

	int statusline_y = area.height - (commandline_line_used ? 2 : 1);
	struct rect statusline_area = {
		.x = area.x,
		.y = statusline_y,
		.width = area.width,
		.height = 1,
	};
	render_statusline(e, fb, statusline_area);

	// render commandline
	struct rect cmdline_area = { .x = area.x, .y = area.y + area.height - 1, .width = area.width, .height = 1 };
	if (e->mode == MODE_COMMAND) {
		render_str(fb, cmdline_area, commandline_prompt, GUTTER_STYLE);
		cmdline_area.x += commandline_prompt.len;
		cmdline_area.width = e->commandline.len;
		render_str(fb, cmdline_area, string_as_str(e->commandline), NORMAL_STYLE);
	} else if (e->errormsg.len > 0) {
		render_str(fb, cmdline_area, string_as_str(e->errormsg), ERRORMSG_STYLE);
	}

	struct rect mainview_area = {
		.x = area.x,
		.y = area.y,
		.width = area.width,
		.height = area.height - (commandline_line_used ? 2 : 1),
	};
	pane_render(&e->foo, fb, mainview_area);
}

static void editor_handle_normal_mode_keyevt(struct editor *e, struct keyevt evt) {
	if (EVT_IS_CHAR(evt, ' ')) {
		string_clear(&e->commandline);
		string_clear(&e->errormsg);
		e->mode = MODE_COMMAND;
		return;
	}

	if (EVT_IS_CHAR(evt, 'i')) {
		e->mode = MODE_INSERT;
		return;
	}

	if (EVT_IS_CHAR(evt, '0')) {
		e->foo.cursor_line_idx = 0;
		return;
	}

	if (EVT_IS_CHAR(evt, 'a')) {
		e->foo.cursor_line_idx = MIN(e->foo.cursor_line->string.len, e->foo.cursor_line_idx + 1);
		e->mode = MODE_INSERT;
		return;
	}

	if (EVT_IS_CHAR(evt, 'h')) {
		e->foo.cursor_line_idx = e->foo.cursor_line_idx > 0 ? e->foo.cursor_line_idx - 1 : 0;
		return;
	}

	if (EVT_IS_CHAR(evt, 'j')) {
		struct pane *curp = &e->foo;
		if (curp->cursor_line->next != NULL) {
			curp->cursor_line = curp->cursor_line->next;
			curp->cursor_line_idx = MIN(curp->cursor_line_idx, curp->cursor_line->string.len - 1);
		}
		return;
	}

	if (EVT_IS_CHAR(evt, 'k')) {
		struct pane *curp = &e->foo;
		if (curp->cursor_line->prev != NULL) {
			curp->cursor_line = curp->cursor_line->prev;
			curp->cursor_line_idx = MIN(curp->cursor_line_idx, curp->cursor_line->string.len - 1);
		}
		return;
	}

	if (EVT_IS_CHAR(evt, 'l')) {
		e->foo.cursor_line_idx = MIN(e->foo.cursor_line->string.len - 1, e->foo.cursor_line_idx + 1);
		return;
	}

	if (EVT_IS_CHAR(evt, 'A')) {
		e->foo.cursor_line_idx = e->foo.cursor_line->string.len;
		e->mode = MODE_INSERT;
		return;
	}
}

static void editor_eval_commandline(struct editor *e, str_t cmd) {
	if (str_eq(cmd, STR("q"))) {
		e->should_exit = 1;
		return;
	}

	char errmsg[1000];
	sprintf(errmsg, "Invalid command: %.*s", (int) cmd.len, cmd.ptr);
	assert(strlen(errmsg) < sizeof(errmsg));
	string_free(e->errormsg);
	e->errormsg = str_to_string((str_t) { .ptr = errmsg, .len = strlen(errmsg) });
}

static void editor_handle_insert_mode_keyevt(struct editor *e, struct keyevt evt) {
	if (evt.kind == KEYKIND_ESCAPE) {
		e->foo.cursor_line_idx = e->foo.cursor_line_idx > 0 ? e->foo.cursor_line_idx - 1 : 0;
		e->mode = MODE_NORMAL;
		return;
	}

	if (evt.kind == KEYKIND_CHAR) {
		struct pane *curp = &e->foo;
		string_insert(&curp->cursor_line->string, curp->cursor_line_idx, evt.kchar);
		curp->cursor_line_idx += 1;
		return;
	}

	if (evt.kind == KEYKIND_DELETE) {
		struct pane *curp = &e->foo;

		if (curp->cursor_line_idx == curp->cursor_line->string.len)
			return;

		string_remove(&curp->cursor_line->string, curp->cursor_line_idx);
		return;
	}

	if (evt.kind == KEYKIND_BACKSPACE) {
		struct pane *curp = &e->foo;

		if (curp->cursor_line_idx == 0)
			return;

		string_remove(&curp->cursor_line->string, curp->cursor_line_idx - 1);
		curp->cursor_line_idx -= 1;
		return;
	}
}

static void editor_handle_command_mode_keyevt(struct editor *e, struct keyevt evt) {
	switch (evt.kind) {
	case KEYKIND_ESCAPE:
		e->mode = MODE_NORMAL;
		break;
	case KEYKIND_CHAR:
		string_push_char(&e->commandline, evt.kchar);
		break;
	case KEYKIND_ENTER:
		e->mode = MODE_NORMAL;
		editor_eval_commandline(e, string_as_str(e->commandline));
		break;
	case KEYKIND_BACKSPACE:
		string_pop_char(&e->commandline);
		break;
	default:
		break;
	}
}

void editor_handle_keyevt(struct editor *e, struct keyevt evt) {
	switch (e->mode) {
	case MODE_NORMAL:
		editor_handle_normal_mode_keyevt(e, evt);
		break;
	case MODE_COMMAND:
		editor_handle_command_mode_keyevt(e, evt);
		break;
	case MODE_INSERT:
		editor_handle_insert_mode_keyevt(e, evt);
		break;
	}
}
