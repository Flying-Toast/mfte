#ifndef __HAVE_BUFLINES_H
#define __HAVE_BUFLINES_H

#include "mf_string.h"

// a single line in a pane buffer
struct bufline {
	string_t string;
	struct bufline *prev;
	struct bufline *next;
};

struct bufline *bufline_new_with_string(string_t s);
void bufline_free(struct bufline *bl);
struct bufline *str_to_buflines(str_t s);
void free_bufline_list(struct bufline *head);

#endif
