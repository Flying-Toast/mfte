#ifndef __HAVE_EDITOR_H
#define __HAVE_EDITOR_H

#include "input.h"
#include "mf_string.h"
#include "render.h"

enum editor_mode {
	MODE_NORMAL,
	MODE_COMMAND,
};

struct pane {
	string_t content;
	unsigned show_line_nums : 1;
};

struct editor {
	enum editor_mode mode;
	string_t commandline;
	unsigned should_exit : 1;
	struct pane foo;
};

void editor_new(struct editor *e);
void editor_free(struct editor *e);
void editor_render(struct editor *e, struct framebuf *fb, struct rect area);
void editor_handle_keyevt(struct editor *e, struct keyevt evt);

#endif
