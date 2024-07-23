#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"

#define CLR_SCREEN "\033[2J"
#define ZERO_CURSOR "\033[1;1H"
#define RESET_FRAME CLR_SCREEN ZERO_CURSOR
#define MIN(a, b) ({ typeof(a) __a = a; typeof(b) __b = b; (__a) < (__b) ? (__a) : (__b); })
#define MAX(a, b) ({ typeof(a) __a = a; typeof(b) __b = b; (__a) > (__b) ? (__a) : (__b); })
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
				sprintf(colorbuf, "\033[38;2;%d;%d;%dm", R_BYTE(pixel.style.fg), G_BYTE(pixel.style.fg), B_BYTE(pixel.style.fg));
				fwrite(colorbuf, 1, strlen(colorbuf), stdout);
			}

			// background
			if ((x == 0 && y == 0) || prevpx.style.bg != pixel.style.bg) {
				sprintf(colorbuf, "\033[48;2;%d;%d;%dm", R_BYTE(pixel.style.bg), G_BYTE(pixel.style.bg), B_BYTE(pixel.style.bg));
				fwrite(colorbuf, 1, strlen(colorbuf), stdout);
			}

			fwrite(&pixel.ch, 1, 1, stdout);
			prevpx = pixel;
		}
	}
}

void framebuf_new(struct framebuf *fb, int width, int height) {
	fb->width = width;
	fb->height = height;
	fb->buf = malloc(sizeof(fb->buf[0]) * width * height);
	fb->bufcap = width * height;
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

	int stridx = 0;

	for (int i = 0; i < MIN(area.width, str.len); i++) {
		struct pixel *px = &fb->buf[area.y * fb->width + area.x + i];
		px->style = style;
		px->ch = str.ptr[stridx++];
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
