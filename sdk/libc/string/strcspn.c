/*
	libc/string/strcspn.c
*/

#include <string.h>

size_t strcspn(const char *s, const char *reject)
{
	const char *p;

	for (p = s; *p != '\0'; ++p)
		if (strchr(reject, *p))
			break;

	return p - s;
}