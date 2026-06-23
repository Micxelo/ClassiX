/*
	libc/string/memcpy.c
*/

#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t count)
{
	char *tmp = dest;
	const char *s = src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}