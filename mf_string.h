#ifndef __HAVE_MF_STRING_H
#define __HAVE_MF_STRING_H

#include <stddef.h>

#define STR(LIT) (str_t) { .ptr = "" LIT "", .len = sizeof("" LIT "") - 1 }

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
void string_push_char(string_t *s, char ch);
void string_clear(string_t *s);
void string_pop_char(string_t *s);
str_t string_as_str(string_t s);
int str_eq(str_t a, str_t b);

#endif
