#ifndef __HAVE_FRAMEBUF_H
#define __HAVE_FRAMEBUF_H

#include <stdint.h>
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

void render_solid_color(struct framebuf *fb, struct rect area, uint32_t color);
void render_string(struct framebuf *fb, struct rect area, char *str, struct style style);

#endif
