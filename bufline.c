#include <stdlib.h>
#include "bufline.h"

struct bufline *bufline_new_with_string(string_t s) {
	struct bufline *ret = malloc(sizeof(struct bufline));
	ret->string = s;
	ret->prev = NULL;
	ret->next = NULL;

	return ret;
}

void bufline_free(struct bufline *bl) {
	string_free(bl->string);
	free(bl);
}

void free_bufline_list(struct bufline *head) {
	while (head != NULL) {
		struct bufline *next = head->next;
		bufline_free(head);
		head = next;
	}
}

struct bufline *multiline_string_to_buflines(string_t s) {
	if (str_is_empty(string_as_str(s)))
		return NULL;

	struct bufline *prev = bufline_new_with_string(
		str_to_string(str_slice_idx_to_eol(string_as_str(s), 0))
	);
	size_t idx = prev->string.len + 1;
	struct bufline *ret = prev;
	struct bufline *head = NULL;

	while (idx < s.len) {
		head = bufline_new_with_string(
			str_to_string(str_slice_idx_to_eol(string_as_str(s), idx))
		);
		idx += head->string.len + 1;
		prev->next = head;
		head->prev = prev;
		prev = head;
	}

	return ret;
}
