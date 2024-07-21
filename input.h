#ifndef __HAVE_INPUT_H
#define __HAVE_INPUT_H

enum keykind {
	KEYKIND_ENTER,
	KEYKIND_TAB,
	// a printable character
	KEYKIND_CHAR,
	KEYKIND_BACKSPACE,
	// <esc>
	KEYKIND_ESCAPE,
	KEYKIND_HOME,
	KEYKIND_END,
	KEYKIND_DELETE,
};

struct keyevt {
	// union tag
	enum keykind kind;
	union {
		// kind == KEYKIND_CHAR
		char kchar;
	};
	// ctrl key held?
	unsigned ctrl : 1;
};

int input_try_get_keyevt(struct keyevt *ret);

#endif
