/*
	libc/string/strcmp.c
*/

#include <string.h>

int strcmp(const char *cs, const char *ct)
{
	char c1, c2;

	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}

	return 0;
}