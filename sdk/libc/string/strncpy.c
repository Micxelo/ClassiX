/*
	libc/string/strncpy.c
*/

#include <string.h>

char *strncpy(char *restrict dest, const char *restrict src, size_t count)
{
	char *tmp = dest;

	while (count) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		count--;
	}

	return dest;
}