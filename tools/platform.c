/*
 * Copyright (c) 2025 Jacob Lifshay <programmerjake@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

/* utility functions that aren't available on all platforms, so we reimplement them here */

#include <ctype.h>
#include <string.h>

#include "platform.h"

char *strsep_(char **restrict stringp, const char *restrict delim)
{
	char *s = *stringp;
	char *retval = s;
	if (!retval)
		return retval;
	for (; *s; s++) {
		for (const char *d = delim; *d; d++) {
			if (*s == *d) {
				*s++ = '\0';
				*stringp = s;
				return retval;
			}
		}
	}
	*stringp = NULL;
	return retval;
}

int strncasecmp_(const char *s1, const char *s2, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		int c1 = tolower((unsigned char) s1[i]);
		int c2 = tolower((unsigned char) s2[i]);
		if (c1 < c2)
			return -1;
		if (c1 > c2)
			return 1;
		if (c1 == '\0')
			break;
	}
	return 0;
}

char *strcasestr_(const char *haystack, const char *needle)
{
	size_t haystack_len = strlen(haystack);
	size_t needle_len = strlen(needle);
	for (; haystack_len >= needle_len; haystack++, haystack_len--) {
		if (strncasecmp_(haystack, needle, needle_len) == 0)
			return (char *) haystack;
	}
	return NULL;
}
