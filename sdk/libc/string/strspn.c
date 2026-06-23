/*
	libc/string/strspn.c
*/

#include <string.h>

size_t strspn(const char *s, const char *accept)
{
	const char *p;

	for (p = s; *p != '\0'; ++p)
		if (!strchr(accept, *p))
			break;

	return p - s;
}