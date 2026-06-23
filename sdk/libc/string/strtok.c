/*
	libc/string/strtok.c
*/

#include <string.h>

char *strtok(char *restrict s, const char *restrict delim)
{
	static char *p;
	if (!s && !(s = p))
		return NULL;

	s += strspn(s, delim);
	if (!*s)
		return p = 0;

	p = s + strcspn(s, delim);
	if (*p)
		*p++ = 0;
	else
		p = 0;

	return s;
}
