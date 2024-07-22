#include <stdlib.h>
#include <string.h>
#include "mf_string.h"

static void string_reserve(string_t *s, size_t cap) {
	if (s->cap >= cap)
		return;
	s->ptr = realloc(s->ptr, cap);
	s->cap = cap;
}

string_t string_new(void) {
	return (string_t) {
		.cap = 1,
		.ptr = malloc(1),
		.len = 0,
	};
}

void string_free(string_t s) {
	free(s.ptr);
}

void string_push_char(string_t *s, char ch) {
	string_reserve(s, s->len + 1);
	s->ptr[s->len++] = ch;
}

void string_pop_char(string_t *s) {
	if (s->len > 0)
		s->len--;
}

void string_clear(string_t *s) {
	s->len = 0;
}

str_t string_as_str(string_t s) {
	return (str_t) {
		.len = s.len,
		.ptr = s.ptr,
	};
}

int str_eq(str_t a, str_t b) {
	if (a.len != b.len)
		return 0;

	return memcmp(a.ptr, b.ptr, a.len) == 0;
}

string_t str_to_string(str_t s) {
	string_t ret = {
		.ptr = malloc(s.len),
		.len = s.len,
		.cap = s.len,
	};
	memcpy(ret.ptr, s.ptr, s.len);
	return ret;
}
