#ifndef __HAVE_FRAMEBUF_H
#define __HAVE_FRAMEBUF_H

#include <stddef.h>
#include <stdint.h>
#include "mf_string.h"
#include "style.h"

#define BAR_CURSOR_ESC "\033[6 q"
#define BLOCK_CURSOR_ESC "\033[2 q"

#define MIN(a, b) ({ typeof(a) __a = a; typeof(b) __b = b; (__a) < (__b) ? (__a) : (__b); })
#define MAX(a, b) ({ typeof(a) __a = a; typeof(b) __b = b; (__a) > (__b) ? (__a) : (__b); })

struct style {
	uint32_t fg;
	uint32_t bg;
};

struct pixel {
	char ch;
	struct style style;
};

struct framebuf {
	int width;
	int height;
	struct pixel *buf;
	size_t bufcap;
};

struct rect {
	// x coord of top left corner
	int x;
	// y coord of top left corner
	int y;
	int width;
	int height;
};

void framebuf_display(struct framebuf *fb);
void framebuf_reset(struct framebuf *fb, int width, int height);
void framebuf_new(struct framebuf *fb, int width, int height);
void framebuf_free(struct framebuf *fb);

struct rect rect_intersect(struct rect a, struct rect b);
struct rect framebuf_intersect(struct framebuf *fb, struct rect area);
int rect_empty(struct rect r);

void render_solid_color(struct framebuf *fb, struct rect area, uint32_t color);
void render_str(struct framebuf *fb, struct rect area, str_t str, struct style style);

#endif
