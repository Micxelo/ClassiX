/*
	libc/string/strrchr.c
*/

#include <string.h>

char *strrchr(const char *s, int c)
{
	const char *last = NULL;

	do {
		if (*s == c)
			last = s;
	} while (*s++);

	return (char *) last;
}