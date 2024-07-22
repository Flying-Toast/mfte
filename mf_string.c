#include <stdlib.h>
#include <string.h>
#include "mf_string.h"

void string_free(string_t s) {
	free(s.ptr);
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

str_t string_as_str(string_t s) {
	return (str_t) {
		.ptr = s.ptr,
		.len = s.len,
	};
}
