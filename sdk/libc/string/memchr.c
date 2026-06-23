/*
	libc/string/memchr.c
*/

#include <string.h>

void *memchr(void *s, int c, size_t n)
{
	const unsigned char *p = s;

	while (n-- != 0)
		if (c == *p++)
			return (void *) (p - 1);

	return NULL;
}