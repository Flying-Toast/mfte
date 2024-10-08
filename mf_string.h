#ifndef __HAVE_MF_STRING_H
#define __HAVE_MF_STRING_H

#include <stddef.h>

#define STR(LIT) (str_t) { .ptr = "" LIT "", .len = sizeof("" LIT "") - 1 }
#define STRING(LIT) str_to_string(STR(LIT))

typedef struct {
	const char *ptr;
	size_t len;
} str_t;

typedef struct {
	char *ptr;
	size_t len;
	size_t cap;
} string_t;

string_t string_new(void);
void string_free(string_t s);
void string_push(string_t *s, char ch);
void string_clear(string_t *s);
void string_pop(string_t *s);
str_t string_as_str(string_t s);
int str_eq(str_t a, str_t b);
string_t str_to_string(str_t s);
str_t str_slice_idx_to_eol(str_t s, size_t idx);
int str_is_empty(str_t s);
void string_insert(string_t *s, size_t idx, char ch);
void string_remove(string_t *s, size_t idx);
[[nodiscard]] int read_file_to_string(char *path, string_t *s);
void string_append(string_t *s, str_t other);
str_t cstr_as_str(char *cstr);

#endif
