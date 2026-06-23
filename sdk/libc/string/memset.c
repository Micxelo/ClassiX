/*
	libc/string/memset.c
*/

#include <string.h>

void *memset(void *s, int c, size_t count)
{
	char *xs = s;

	while (count--)
		*xs++ = c;

	return s;
}