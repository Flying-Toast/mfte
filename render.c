#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "render.h"

#define CLR_SCREEN "\033[2J"
#define ZERO_CURSOR "\033[1;1H"
#define BAR_CURSOR_ESC "\033[6 q"
#define BLOCK_CURSOR_ESC "\033[2 q"
#define RESET_FRAME CLR_SCREEN ZERO_CURSOR
#define R_BYTE(color) (color >> 16)
#define G_BYTE(color) ((color >> 8) & 0xFF)
#define B_BYTE(color) (color & 0xFF)

void framebuf_display(struct framebuf *fb) {
	fwrite(RESET_FRAME, 1, strlen(RESET_FRAME), stdout);

	struct pixel prevpx;

	for (int y = 0; y < fb->height; y++) {
		for (int x = 0; x < fb->width; x++) {
			struct pixel pixel = fb->buf[y * fb->width + x];
			char colorbuf[sizeof("\033[38;2;XXX;XXX;XXXm")];

			// foreground
			if ((x == 0 && y == 0) || prevpx.style.fg != pixel.style.fg) {
				snprintf(colorbuf, sizeof(colorbuf), "\033[38;2;%d;%d;%dm", R_BYTE(pixel.style.fg), G_BYTE(pixel.style.fg), B_BYTE(pixel.style.fg));
				fwrite(colorbuf, 1, strlen(colorbuf), stdout);
			}

			// background
			if ((x == 0 && y == 0) || prevpx.style.bg != pixel.style.bg) {
				snprintf(colorbuf, sizeof(colorbuf), "\033[48;2;%d;%d;%dm", R_BYTE(pixel.style.bg), G_BYTE(pixel.style.bg), B_BYTE(pixel.style.bg));
				fwrite(colorbuf, 1, strlen(colorbuf), stdout);
			}

			fwrite(&pixel.ch, 1, 1, stdout);
			prevpx = pixel;
		}
	}

	switch (fb->cursor_style) {
	case CURSOR_BLOCK:
		fwrite(BLOCK_CURSOR_ESC, 1, strlen(BLOCK_CURSOR_ESC), stdout);
		break;
	case CURSOR_BAR:
		fwrite(BAR_CURSOR_ESC, 1, strlen(BAR_CURSOR_ESC), stdout);
		break;
	}
	char moveesc[sizeof("\033[XXX;XXXH") - 1] = "";
	snprintf(moveesc, sizeof(moveesc), "\033[%d;%dH", fb->cursory + 1, fb->cursorx + 1);
	fwrite(moveesc, 1, strlen(moveesc), stdout);
}

void framebuf_new(struct framebuf *fb, int width, int height) {
	fb->width = width;
	fb->height = height;
	fb->buf = malloc(sizeof(fb->buf[0]) * width * height);
	fb->bufcap = width * height;
	fb->cursorx = 0;
	fb->cursory = 0;
}

void framebuf_reset(struct framebuf *fb, int width, int height) {
	if (fb->bufcap < width * height) {
		fb->buf = realloc(fb->buf, sizeof(fb->buf[0]) * width * height);
		fb->bufcap = width * height;
	}
	fb->width = width;
	fb->height = height;

	for (int i = 0; i < width * height; i++) {
		fb->buf[i].ch = ' ';
		fb->buf[i].style.fg = WHITE_COLOR;
		fb->buf[i].style.bg = BG_COLOR;
	}
}

void framebuf_free(struct framebuf *fb) {
	free(fb->buf);
}

struct rect rect_intersect(struct rect a, struct rect b) {
	if (
		a.x >= b.x + b.width
		|| a.x + a.width <= b.x
		|| a.y >= b.y + b.height
		|| a.y + a.height <= b.y
	) {
		// no intersection
		return (struct rect) {0};
	}

	struct rect ret;
	ret.x = MAX(a.x, b.x);
	ret.y = MAX(a.y, b.y);
	ret.width = MIN(a.x + a.width - ret.x, b.x + b.width - ret.x);
	ret.height = MIN(a.y + a.height - ret.y, b.y + b.height - ret.y);
	return ret;
}

struct rect framebuf_intersect(struct framebuf *fb, struct rect area) {
	struct rect fb_rect = { .width = fb->width, .height = fb->height };
	return rect_intersect(fb_rect, area);
}

void render_restore_cursor_style(void) {
	fwrite(BLOCK_CURSOR_ESC, 1, strlen(BLOCK_CURSOR_ESC), stdout);
}

int rect_empty(struct rect r) {
	return r.width <= 0 && r.height <= 0;
}

void render_solid_color(struct framebuf *fb, struct rect area, uint32_t color) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	for (int y = area.y; y < area.y + area.height; y++) {
		for (int x = area.x; x < area.x + area.width; x++) {
			struct pixel *p = &fb->buf[y * fb->width + x];
			p->ch = ' ';
			p->style.bg = color;
		}
	}
}

void render_str(struct framebuf *fb, struct rect area, str_t str, struct style style) {
	area = framebuf_intersect(fb, area);
	if (rect_empty(area))
		return;

	int rendered_width = 0;
	for (int stridx = 0; stridx < str.len && rendered_width < area.width; stridx++) {
		struct pixel *px = &fb->buf[area.y * fb->width + area.x + rendered_width];
		if (str.ptr[stridx] == '\t') {
			if (rendered_width + TAB_WIDTH > area.width)
				break;

			for (int j = 0; j < TAB_WIDTH; j++) {
				px->style = style;
				px->ch = ' ';
				px++;
			}

			rendered_width += TAB_WIDTH;
		} else if (isprint(str.ptr[stridx])) {
				px->ch = str.ptr[stridx];
				px->style = style;
				rendered_width += 1;
		} else {
			char hexbuf[5];
			if (rendered_width + sizeof(hexbuf) - 1 > area.width)
				break;

			int slen = snprintf(hexbuf, sizeof(hexbuf), "<%02x>", str.ptr[stridx] & 0xff);
			for (int j = 0; j < sizeof(hexbuf) - 1; j++) {
				px[j].ch = hexbuf[j];
				px[j].style = NONPRINT_STYLE;
			}
			rendered_width += slen;
		}
	}
}

#ifdef MF_BUILD_TESTS
#include <assert.h>

static int rect_eq(struct rect a, struct rect b) {
	return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

static void print_rect(struct rect r) {
	printf("struct rect { .x = %d, .y = %d, .width = %d, .height = %d }\n", r.x, r.y, r.width, r.height);
}

static void rect_assert_eq(struct rect lhs, struct rect rhs) {
	if (!rect_eq(lhs, rhs)) {
		printf("assertion `lhs == rhs` failed.\n");
		printf("lhs: ");
		print_rect(lhs);
		printf("rhs: ");
		print_rect(rhs);
		assert(0);
	}
}

void render_run_tests(void) {
	{
		struct rect a = { .width = 10, .height = 3 };
		struct rect b = { .x = 3, .y = 5, .width = 100, .height = 200 };
		assert(rect_empty(rect_intersect(a, b)));
	}
	{
		struct rect a = { .width = 10, .height = 3 };
		struct rect b = { .x = 3, .y = 1, .width = 100, .height = 200 };
		struct rect expected_inter = { .x = 3, .y = 1, .width = 7, .height = 2 };
		rect_assert_eq(expected_inter, rect_intersect(a, b));
	}

	assert(rect_empty((struct rect) { .x = 12, .y = 1, .width = 0, .height = 0 }));
	assert(rect_empty((struct rect) { .width = -1, .height = -1 }));
	assert(rect_empty((struct rect) { 0 }));
}
#endif
