#include <err.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "editor.h"
#include "render.h"
#include "input.h"

#ifdef MF_BUILD_TESTS
#include <string.h>

void render_run_tests(void);
void mf_string_run_tests(void);

void mf_run_tests(void) {
	render_run_tests();
	mf_string_run_tests();
}
#endif

static struct termios original_termios;

static int term_init(void) {
	struct termios t;

	if (tcgetattr(STDIN_FILENO, &t))
		return -1;

	original_termios = t;

	cfmakeraw(&t);
	t.c_cc[VMIN] = 0;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t))
		return -1;

	// enter alt screen
#define ENTER_ALT "\033[?1049h"
	fwrite(ENTER_ALT, 1, strlen(ENTER_ALT), stdout);

	return 0;
}

static void term_cleanup(void) {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

static struct winsize get_term_size(void) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
		err(1, "ioctl");
	return ws;
}

static int resized_flag;
static void sigwinch_handler(int signo) {
	resized_flag = 1;
}

int main(int argc, char **argv) {
	int stdoutbufsize = 10000;
	char *stdoutbuf = malloc(stdoutbufsize);
	if (setvbuf(stdout, stdoutbuf, _IOFBF, stdoutbufsize))
		err(1, "setvbuf");

#ifdef MF_BUILD_TESTS
	if (argc == 2 && !strcmp(argv[1], "--test")) {
		mf_run_tests();
		return 0;
	}
#endif

	if (term_init())
		err(1, "enable raw mode");
	if (atexit(term_cleanup))
		err(1, "atexit handler");

	struct winsize ws = get_term_size();
	int term_width = ws.ws_col;
	int term_height = ws.ws_row;

	struct sigaction winch_act = { .sa_handler = sigwinch_handler };
	if (sigaction(SIGWINCH, &winch_act, NULL))
		err(1, "sigaction");

	struct editor editor;
	editor_new(&editor);

	struct framebuf fb;
	framebuf_new(&fb, term_width, term_height);
	while (!editor.should_exit) {
		framebuf_reset(&fb, term_width, term_height);
		editor_render(&editor, &fb, (struct rect) { .width = fb.width, .height = fb.height });
		framebuf_display(&fb);

		struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
		int pollret = poll(&pfd, 1, -1);
		// Poll finished. There is either data available on stdin,
		// or poll was interrupted by a signal.
		if (pollret == -1) {
			if (errno == EINTR) {
				// poll was interrupted, likely by SIGWINCH. Handle resize:
				if (resized_flag) {
					ws = get_term_size();
					term_width = ws.ws_col;
					term_height = ws.ws_row;
					resized_flag = 0;
				}
			} else {
				// non-EINTR poll error
				err(1, "poll");
			}
		} else {
			// pollret != -1, so there is data for reading:
			struct keyevt kevt;
			if (input_try_get_keyevt(&kevt) == 0) {
				editor_handle_keyevt(&editor, kevt);
			} else {
				errx(1, "poll returned but no data read");
			}
		}
	}
	framebuf_free(&fb);
	editor_free(&editor);

	// leave alt screen. do this here instead of in term_cleanup(),
	// otherwise err/errx messages won't be visible because they'll
	// be printed on the alternate screen.
#define LEAVE_ALT "\033[?1049l"
	fwrite(LEAVE_ALT, 1, strlen(LEAVE_ALT), stdout);

	fflush(stdout);
	setvbuf(stdout, NULL, _IOFBF, 0);
	free(stdoutbuf);
}
