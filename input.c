#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include "input.h"

static int try_readone(void) {
	char ret;
	int nread = read(STDIN_FILENO, &ret, 1);

	if (nread == 1) {
		return ret;
	} else if (nread == 0 || (nread == -1 && (errno == EAGAIN || errno == EINTR))) {
		return -1;
	} else {
		err(1, "try_readone");
	}
}

static char readone(void) {
	int ret;
	while ((ret = try_readone()) == -1)
		;
	return ret;
}

int input_try_get_keyevt(struct keyevt *ret) {
	int r = try_readone();
	if (r == -1)
		return -1;
	char firstbyte = r;

	// <ESC> or escape sequence
	if (firstbyte == 27) {
		int next = try_readone();
		if (next == -1) {
			ret->kind = KEYKIND_ESCAPE;
			return 0;
		} else if (next != '[') {
			errx(1, "got %d (%c) in escape code, expected '['", next, isprint(next) ? next : '-');
		}

		char seq[2];
		seq[0] = readone();
		int seqsize;

		int opt_fourth = try_readone();
		if (opt_fourth == -1) {
			seqsize = 1;
		} else {
			seq[1] = opt_fourth;
			seqsize = 2;
		}

		if (
			(seqsize == 2 && seq[0] == '1' && seq[1] == '~')
			|| (seqsize == 2 && seq[0] == '7' && seq[1] == '~')
			|| (seqsize == 2 && seq[0] == 'O' && seq[1] == 'H')
			|| (seqsize == 1 && seq[0] == 'H')
		) {
			ret->kind = KEYKIND_HOME;
			return 0;
		}

		if (
			(seqsize == 2 && seq[0] == '4' && seq[1] == '~')
			|| (seqsize == 2 && seq[0] == '8' && seq[1] == '~')
			|| (seqsize == 2 && seq[0] == 'O' && seq[1] == 'F')
			|| (seqsize == 1 && seq[0] == 'F')
		) {
			ret->kind = KEYKIND_END;
			return 0;
		}

		if (seqsize == 2 && seq[0] == '3' && seq[1] == '~') {
			ret->kind = KEYKIND_DELETE;
			return 0;
		}

		errx(1, "escape seq unreachable");
	}

	switch (firstbyte) {
	case 13:
		ret->kind = KEYKIND_ENTER;
		return 0;
	case 9:
		ret->kind = KEYKIND_TAB;
		return 0;
	case 127:
		ret->kind = KEYKIND_BACKSPACE;
		return 0;
	}

	// ctrl+[a-z] (except ctrl+m=ENTER and ctrl+i=TAB)
	if (firstbyte >= 1 && firstbyte <= 26) {
		ret->kind = KEYKIND_CHAR;
		ret->kchar = 'a' - 1 + firstbyte;
		ret->ctrl = 1;
		return 0;
	}

	if (isprint(firstbyte)) {
		ret->kind = KEYKIND_CHAR;
		ret->kchar = firstbyte;
		return 0;
	}

	errx(1, "%s unreachable", __func__);
}
