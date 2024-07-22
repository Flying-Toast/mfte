#include <err.h>
#include <string.h>
#include "editor.h"

static void pane_new(struct pane *p) {
	p->content = STRING("WOOOOHOOO hello there this is some text\nand it is in a pane :-)\nthis line is really jfaalew jfoiewaj oifjaweoijflaonge ong f jaewjf oiawejf oiajewoif jaoiewjf oiajewf043aj9f43aj09j 09j09");
	p->show_line_nums = 1;
}

static void pane_free(struct pane *p) {
	string_free(p->content);
}

void editor_new(struct editor *e) {
	e->mode = MODE_NORMAL;
	e->commandline = string_new();
	e->should_exit = 0;
	pane_new(&e->foo);
}

void editor_free(struct editor *e) {
	pane_free(&e->foo);
	string_free(e->commandline);
}

static void render_flowed_text(struct framebuf *fb, struct rect area, str_t text) {
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
		struct style sty = { .fg = YELLOW_COLOR, .bg = PURPLE_COLOR };
		str_t line = str_slice_idx_to_eol(text, line_start_idx);
		line_start_idx += line.len + 1;
		render_str(fb, line_area, line, sty);
	}
}

static void pane_render(struct pane *p, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	struct rect line_num_area = { .x = area.x, .y = area.y };
	struct rect content_area = area;
	if (p->show_line_nums) {
		line_num_area.height = area.height;
		line_num_area.width = 4;
		content_area.width -= line_num_area.width;
		content_area.x += line_num_area.width;
	}

	render_flowed_text(fb, content_area, string_as_str(p->content));
}

static void render_statusline(struct editor *e, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	render_solid_color(fb, area, LIGHTBG_COLOR);

	struct style modestyle = { .fg = BG_COLOR };
	str_t modestr;

	switch (e->mode) {
	case MODE_NORMAL:
		modestyle.bg = GREEN_COLOR;
		modestr = STR(" NORMAL ");
		break;
	case MODE_COMMAND:
		modestyle.bg = GREEN_COLOR;
		modestr = STR(" COMMAND ");
		break;
	default:
		errx(1, "mode switch unreachable (%d)", e->mode);
	}

	struct rect mode_area = { .x = area.x, .y = area.y, .width = modestr.len, .height = 1 };
	render_str(fb, mode_area, modestr, modestyle);
}

void editor_render(struct editor *e, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	int is_commandmode = e->mode == MODE_COMMAND;

	int statusline_y = area.height - (is_commandmode ? 2 : 1);
	struct rect statusline_area = {
		.x = area.x,
		.y = statusline_y,
		.width = area.width,
		.height = 1,
	};
	render_statusline(e, fb, statusline_area);

	// render commandline
	if (is_commandmode) {
		struct style cmd_style = { .fg = WHITE_COLOR, .bg = BG_COLOR };
		struct rect cmd_area = { .x = area.x, .y = area.y + area.height - 1, .width = 2, .height = 1 };
		render_str(fb, cmd_area, STR("> "), cmd_style);
		cmd_area.x += 2;
		cmd_area.width = e->commandline.len;
		render_str(fb, cmd_area, string_as_str(e->commandline), cmd_style);
	}

	struct rect mainview_area = {
		.x = area.x,
		.y = area.y,
		.width = area.width,
		.height = area.height - (is_commandmode ? 2 : 1),
	};
	pane_render(&e->foo, fb, mainview_area);
}

static void editor_handle_normal_mode_keyevt(struct editor *e, struct keyevt evt) {
	if (EVT_IS_CHAR(evt, ' ')) {
		string_clear(&e->commandline);
		e->mode = MODE_COMMAND;
		return;
	}
}

static void editor_eval_commandline(struct editor *e, str_t cmd) {
	if (str_eq(cmd, STR("q")))
		e->should_exit = 1;
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
	default:
		errx(1, "mode switch unreachable (%d)", e->mode);
	}
}
