#ifndef __HAVE_EDITOR_H
#define __HAVE_EDITOR_H

#include "bufline.h"
#include "input.h"
#include "mf_string.h"
#include "render.h"

enum editor_mode {
	MODE_NORMAL,
	MODE_COMMAND,
	MODE_INSERT,
};

struct pane {
	// line the cursor is on
	struct bufline *cursor_line;
	// index into `cursor_line->string` of the cursor
	size_t cursor_line_idx;
	unsigned show_line_nums : 1;
	// name displayed in statusline
	string_t name;
};

struct editor {
	enum editor_mode mode;
	string_t commandline;
	unsigned should_exit : 1;
	struct pane foobar123lol; // temporary :-)
	string_t errormsg;
};

void editor_new(struct editor *e, str_t initial_contents);
void editor_free(struct editor *e);
void editor_render(struct editor *e, struct framebuf *fb, struct rect area);
void editor_handle_keyevt(struct editor *e, struct keyevt evt);
struct pane *editor_get_focused_pane(struct editor *e);

#endif
