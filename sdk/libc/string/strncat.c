/*
	libc/string/strncat.c
*/

#include <string.h>

char *strncat(char *restrict dest, const char *restrict src, size_t count)
{
	char *tmp = dest;

	if (count) {
		while (*dest)
			dest++;
		while ((*dest++ = *src++) != 0) {
			if (--count == 0) {
				*dest = '\0';
				break;
			}
		}
	}

	return tmp;
}