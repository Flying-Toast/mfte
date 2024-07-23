#include <assert.h>
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
	if (s.cap > 0)
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

str_t str_slice_idx_to_eol(str_t s, size_t start_idx) {
	str_t ret = { .len = 0, .ptr = s.ptr + start_idx };

	while (start_idx + ret.len < s.len && s.ptr[start_idx + ret.len] != '\n') {
		ret.len++;
	}

	return ret;
}

int str_is_empty(str_t s) {
	return s.len == 0;
}

void string_insert(string_t *s, size_t idx, char ch) {
	assert(idx < s->len);

	string_reserve(s, s->len + 1);
	memmove(s->ptr + idx + 1, s->ptr + idx, s->len - idx);
	s->len += 1;
	s->ptr[idx] = ch;
}

void string_remove(string_t *s, size_t idx) {
	assert(idx < s->len);

	if (idx == s->len - 1) {
		string_pop_char(s);
		return;
	}

	memmove(s->ptr + idx, s->ptr + idx + 1, s->len - idx);
	s->len -= 1;
}

#ifdef MF_BUILD_TESTS
#include <assert.h>
#include <stdio.h>

static void str_assert_eq(str_t lhs, str_t rhs) {
	if (lhs.len != rhs.len || memcmp(lhs.ptr, rhs.ptr, lhs.len) != 0) {
		printf("assertion `lhs == rhs` failed.\n");
		printf("lhs: \"%.*s\"\nrhs: \"%.*s\"\n", (int) lhs.len, lhs.ptr, (int) rhs.len, rhs.ptr);
		assert(0);
	}
}

void mf_string_run_tests(void) {
	str_t s = STR("Hello world\nsecond line\n");
	str_assert_eq(str_slice_idx_to_eol(s, 0), STR("Hello world"));
	str_assert_eq(str_slice_idx_to_eol(s, 2), STR("llo world"));
	str_assert_eq(str_slice_idx_to_eol(s, 11), STR(""));
	str_assert_eq(str_slice_idx_to_eol(s, 12), STR("second line"));

	str_assert_eq(str_slice_idx_to_eol(STR(""), 123), STR(""));

	string_t insert_into_me = STRING("Hello!");
	string_insert(&insert_into_me, 1, 'p');
	str_assert_eq(string_as_str(insert_into_me), STR("Hpello!"));
	string_remove(&insert_into_me, 6);
	str_assert_eq(string_as_str(insert_into_me), STR("Hpello"));
	string_remove(&insert_into_me, 0);
	str_assert_eq(string_as_str(insert_into_me), STR("pello"));
}
#endif
