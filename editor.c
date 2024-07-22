#include <err.h>
#include <string.h>
#include "editor.h"

void editor_init(struct editor *e) {
	e->mode = MODE_NORMAL;
}

static void render_statusline(struct editor *e, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	render_solid_color(fb, area, LIGHTBG_COLOR);

	uint32_t modebg;
	str_t modestr;
	switch (e->mode) {
	case MODE_NORMAL:
		modebg = GREEN_COLOR;
		modestr = STR(" NORMAL ");
		break;
	case MODE_COMMAND:
		modebg = GREEN_COLOR;
		modestr = STR(" COMMAND ");
		break;
	default:
		errx(1, "mode switch unreachable (%d)", e->mode);
	}

	struct rect mode_area = { .x = area.x, .y = area.y, .width = modestr.len, .height = 1 };
	render_str(fb, mode_area, modestr, (struct style) { .fg = BG_COLOR, .bg = modebg });
}

void editor_render(struct editor *e, struct framebuf *fb, struct rect area) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	int statusline_y = area.height - (e->mode == MODE_COMMAND ? 2 : 1);
	struct rect statusline_area = {
		.x = area.x,
		.y = statusline_y,
		.width = area.width,
		.height = 1,
	};
	render_statusline(e, fb, statusline_area);
}

static void editor_handle_normal_mode_keyevt(struct editor *e, struct keyevt evt) {
	if (EVT_IS_CHAR(evt, ' ')) {
		e->mode = MODE_COMMAND;
		return;
	}
}

static void editor_handle_command_mode_keyevt(struct editor *e, struct keyevt evt) {
	if (evt.kind == KEYKIND_ESCAPE) {
		e->mode = MODE_NORMAL;
		return;
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
