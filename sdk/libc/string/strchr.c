/*
	libc/string/strchr.c
*/

#include <string.h>

char *strchr(const char *s, int c)
{
	for (; *s != c; ++s)
		if (*s == '\0')
			return NULL;

	return (char *) s;
}