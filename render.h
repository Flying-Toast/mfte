#ifndef __HAVE_FRAMEBUF_H
#define __HAVE_FRAMEBUF_H

#include <stddef.h>
#include <stdint.h>
#include "mf_string.h"
#include "style.h"

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
void framebuf_init(struct framebuf *fb, int width, int height);
void framebuf_put(struct framebuf *fb);

struct rect rect_intersect(struct rect a, struct rect b);
struct rect framebuf_intersect(struct framebuf *fb, struct rect area);
int rect_empty(struct rect r);

void render_solid_color(struct framebuf *fb, struct rect area, uint32_t color);
void render_str(struct framebuf *fb, struct rect area, str_t str, struct style style);

#endif
