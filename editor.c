#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "editor.h"

static str_t commandline_prompt = STR(">> ");

static void pane_new(struct pane *p, str_t initial_contents) {
	p->_priv_cursor_line = str_to_buflines(initial_contents);
	p->screen_top_line = p->_priv_cursor_line;
	p->_priv_cursor_line_no = 1;
	p->cursor_line_idx = 0;
	p->show_line_nums = 1;
	p->name = STRING("[No Name]");
}

static struct bufline *pane_get_cursor_line(struct pane *p) {
	return p->_priv_cursor_line;
}

struct pane *editor_get_focused_pane(struct editor *e) {
	return &e->foobar123lol;
}

// line number of the line cursor is on
static size_t pane_get_cursor_line_no(struct pane *p) {
	return p->_priv_cursor_line_no;
}

static void pane_free(struct pane *p) {
	struct bufline *first_line;
	for (first_line = pane_get_cursor_line(p); first_line->prev; first_line = first_line->prev)
		;
	string_free(p->name);
	free_bufline_list(first_line);
}

void editor_new(struct editor *e, str_t initial_contents) {
	e->mode = MODE_NORMAL;
	e->commandline = string_new();
	e->errormsg = string_new();
	e->should_exit = 0;
	pane_new(&e->foobar123lol, initial_contents);
}

void editor_free(struct editor *e) {
	pane_free(&e->foobar123lol);
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

static void editor_render_cursor(struct editor *e, struct framebuf *fb, struct rect editor_area) {
	switch (e->mode) {
	case MODE_NORMAL:
		// cursorx/cursory set in pane_render()
		fb->cursor_style = CURSOR_BLOCK;
		break;
	case MODE_INSERT:
		// cursorx/cursory set in pane_render()
		fb->cursor_style = CURSOR_BAR;
		break;
	case MODE_COMMAND:
		fb->cursor_style = CURSOR_BAR;
		fb->cursorx = commandline_prompt.len + e->commandline.len;
		fb->cursory = editor_area.y + editor_area.height;
		break;
	}

}

// factor in tab width, nonprint characters "<XX>" width, ...
static int cursor_idx_to_col(str_t cursor_line, size_t cursor_idx) {
	int ret = 0;
	for (int i = 0; i < cursor_idx; i++) {
		// TODO: more generic/less "hardcodey" way of handling >1-wide characters
		if (cursor_line.ptr[i] == '\t') {
			ret += TAB_WIDTH;
		} else if (isprint(cursor_line.ptr[i])) {
			ret += 1;
		} else {
			ret += sizeof("<XX>") - 1;
		}
	}
	return ret;
}

static void pane_line_up(struct pane *p) {
	if (p->_priv_cursor_line->prev != NULL) {
		p->_priv_cursor_line = p->_priv_cursor_line->prev;
		if (p->_priv_cursor_line->string.len == 0) {
			p->cursor_line_idx = 0;
		} else {
			p->cursor_line_idx = MIN(p->cursor_line_idx, p->_priv_cursor_line->string.len - 1);
		}
		p->_priv_cursor_line_no -= 1;
	}
}

static void pane_line_down(struct pane *p) {
	if (p->_priv_cursor_line->next != NULL) {
		p->_priv_cursor_line = p->_priv_cursor_line->next;
		if (p->_priv_cursor_line->string.len == 0) {
			p->cursor_line_idx = 0;
		} else {
			p->cursor_line_idx = MIN(p->cursor_line_idx, p->_priv_cursor_line->string.len - 1);
		}
		p->_priv_cursor_line_no += 1;
	}
}

static void pane_render(struct pane *p, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	// TODO: proper viewport scrolling
	p->screen_top_line = pane_get_cursor_line(p);

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
	{
		str_t str = string_as_str(pane_get_cursor_line(p)->string);
		fb->cursorx = line_area.x + cursor_idx_to_col(str, p->cursor_line_idx);
	}
	fb->cursory = line_area.y;

	struct rect line_num_area = gutter_area;
	line_num_area.height = 1;

	int cursorline_screen_y = 0;
	for (struct bufline *bl = p->screen_top_line; bl != NULL; bl = bl->next) {
		if (line_area.y >= content_area.height)
			break;

		char linenum[10];
		if (bl == pane_get_cursor_line(p)) {
			snprintf(linenum, sizeof(linenum), "%-3zu ", pane_get_cursor_line_no(p));
			fb->cursory += cursorline_screen_y;
		} else {
			snprintf(linenum, sizeof(linenum), "%3d ", cursorline_screen_y + 1);
		}
		cursorline_screen_y += 1;

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

	struct pane *curp = editor_get_focused_pane(e);

	struct rect name_area = {
		.x = area.x + mode_area.width,
		.y = area.y,
		// +2 for an extra ' ' on both sides
		.width = curp->name.len + 2,
		.height = 1,
	};
	render_solid_color(fb, name_area, STATUSLINE_SECONDARY_STYLE.bg);
	name_area.x += 1;
	render_str(fb, name_area, string_as_str(curp->name), STATUSLINE_SECONDARY_STYLE);
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

	struct rect cmdline_area = {
		.x = area.x,
		.y = area.y + area.height - 1,
		.width = area.width,
		.height = 1
	};
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
	pane_render(&e->foobar123lol, fb, mainview_area);

	// render cursor last, because pane_render() can set cursorx/cursory for e.g. normal mode.
	// it doesn't matter that the cursor gets moved during rendering; fb->cursor(x|y) just stores
	// the position and then actually moves the cursor as the final step of rendering in
	// framebuf_display().
	editor_render_cursor(e, fb, area);
}

static void editor_handle_normal_mode_keyevt(struct editor *e, struct keyevt evt) {
	struct pane *curp = editor_get_focused_pane(e);

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

	if (EVT_IS_CHAR(evt, 'x')) {
		struct bufline *curlin = pane_get_cursor_line(curp);
		if (curlin->string.len == 0)
			return;

		string_remove(&curlin->string, curp->cursor_line_idx);
		curp->cursor_line_idx = MIN(curlin->string.len - 1, curp->cursor_line_idx);
		return;
	}

	if (EVT_IS_CHAR(evt, '0')) {
		curp->cursor_line_idx = 0;
		return;
	}

	if (EVT_IS_CHAR(evt, 'a')) {
		curp->cursor_line_idx = MIN(pane_get_cursor_line(curp)->string.len, curp->cursor_line_idx + 1);
		e->mode = MODE_INSERT;
		return;
	}

	if (EVT_IS_CHAR(evt, 'h')) {
		curp->cursor_line_idx = curp->cursor_line_idx > 0 ? curp->cursor_line_idx - 1 : 0;
		return;
	}

	if (EVT_IS_CHAR(evt, 'j')) {
		pane_line_down(curp);
		return;
	}

	if (EVT_IS_CHAR(evt, 'k')) {
		pane_line_up(curp);
		return;
	}

	if (EVT_IS_CHAR(evt, 'l')) {
		struct bufline *curlin = pane_get_cursor_line(curp);
		curp->cursor_line_idx = MIN(curlin->string.len - 1, curp->cursor_line_idx + 1);
		return;
	}

	if (EVT_IS_CHAR(evt, 'A')) {
		curp->cursor_line_idx = pane_get_cursor_line(curp)->string.len;
		e->mode = MODE_INSERT;
		return;
	}

	if (EVT_IS_CHAR(evt, 'G')) {
		struct bufline *l = pane_get_cursor_line(curp);

		while (l->next != NULL) {
			pane_line_down(curp);
			l = pane_get_cursor_line(curp);
		}

		curp->cursor_line_idx = MIN(curp->cursor_line_idx, l->string.len - 1);
	}

	if (EVT_IS_CHAR(evt, 'o')) {
		struct bufline *cur = pane_get_cursor_line(curp);
		struct bufline *new = bufline_new_with_string(string_new());
		new->next = cur->next;
		new->prev = cur;
		cur->next = new;
		curp->cursor_line_idx = 0;
		pane_line_down(curp);
		e->mode = MODE_INSERT;
	}
}

static void editor_eval_commandline(struct editor *e, str_t cmd) {
	if (str_eq(cmd, STR("q"))) {
		e->should_exit = 1;
		return;
	}

	char errmsg[1000];
	snprintf(errmsg, sizeof(errmsg), "Invalid command: %.*s", (int) cmd.len, cmd.ptr);
	assert(strlen(errmsg) < sizeof(errmsg));
	string_clear(&e->errormsg);
	string_append(&e->errormsg, cstr_as_str(errmsg));
}

static void editor_handle_insert_mode_keyevt(struct editor *e, struct keyevt evt) {
	struct pane *curp = editor_get_focused_pane(e);

	if (evt.kind == KEYKIND_ESCAPE) {
		curp->cursor_line_idx = curp->cursor_line_idx > 0 ? curp->cursor_line_idx - 1 : 0;
		e->mode = MODE_NORMAL;
		return;
	}

	if (evt.kind == KEYKIND_CHAR) {
		struct bufline *curlin = pane_get_cursor_line(curp);
		string_insert(&curlin->string, curp->cursor_line_idx, evt.kchar);
		curp->cursor_line_idx += 1;
		return;
	}

	if (evt.kind == KEYKIND_DELETE) {
		if (curp->cursor_line_idx == pane_get_cursor_line(curp)->string.len)
			return;

		string_remove(&pane_get_cursor_line(curp)->string, curp->cursor_line_idx);
		return;
	}

	if (evt.kind == KEYKIND_BACKSPACE) {
		if (curp->cursor_line_idx == 0)
			return;

		string_remove(&pane_get_cursor_line(curp)->string, curp->cursor_line_idx - 1);
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
		string_push(&e->commandline, evt.kchar);
		break;
	case KEYKIND_ENTER:
		e->mode = MODE_NORMAL;
		editor_eval_commandline(e, string_as_str(e->commandline));
		break;
	case KEYKIND_BACKSPACE:
		string_pop(&e->commandline);
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
