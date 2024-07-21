#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "render.h"
#include "input.h"

#ifdef MF_BUILD_TESTS
#include <string.h>

void render_run_tests(void);

void mf_run_tests(void) {
	render_run_tests();
}
#endif

static struct termios original_termios;

static int enable_rawmode(void) {
	struct termios t;

	if (tcgetattr(STDIN_FILENO, &t))
		return -1;

	original_termios = t;

	cfmakeraw(&t);
	t.c_cc[VMIN] = 0;
	t.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t))
		return -1;

	return 0;
}

static void disable_rawmode(void) {
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
#ifdef MF_BUILD_TESTS
	if (argc == 2 && !strcmp(argv[1], "--test")) {
		mf_run_tests();
		return 0;
	}
#endif

	if (enable_rawmode())
		err(1, "enable raw mode");
	if (atexit(disable_rawmode))
		err(1, "atexit handler");

	int iobufsize = 10000;
	if (setvbuf(stdout, malloc(iobufsize), _IOFBF, iobufsize))
		err(1, "setvbuf");

	struct winsize ws = get_term_size();
	int term_width = ws.ws_col;
	int term_height = ws.ws_row;

	struct sigaction winch_act = { .sa_handler = sigwinch_handler };
	if (sigaction(SIGWINCH, &winch_act, NULL))
		err(1, "sigaction");

	struct framebuf fb;
	framebuf_init(&fb, term_width, term_height);
	for (;;) {
		framebuf_reset(&fb, term_width, term_height);
		render_solid_color(&fb, (struct rect) { .x = 5, .y = 5, .width = 10, .height = 2 }, RED_COLOR);
		render_string(&fb, (struct rect) { .x = 0, .y = 3, .width = 10, .height = 1 }, "Hello!", (struct style) { .bg = BG_COLOR, .fg = GREEN_COLOR });
		framebuf_display(&fb);

		for (;;) {
			if (resized_flag) {
				ws = get_term_size();
				term_width = ws.ws_col;
				term_height = ws.ws_row;
				resized_flag = 0;
				break;
			}

			struct keyevt kevt;
			if (input_try_get_keyevt(&kevt) == 0) {
				// editor_handle_keyevt(editor, kevt);
				break;
			}
		}
	}
	framebuf_put(&fb);
}
