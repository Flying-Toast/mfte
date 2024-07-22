#include <err.h>
#include <string.h>
#include "editor.h"

void editor_init(struct editor *e) {
	e->mode = MODE_NORMAL;
	e->commandline = string_new();
	e->should_exit = 0;
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
