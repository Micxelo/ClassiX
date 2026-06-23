/*
	libc/string/strxfrm.c
*/

#include <string.h>

size_t strxfrm(char *restrict dest, const char *restrict src, size_t count)
{
	size_t len = strlen(src);
	if (count > len)
		strcpy(dest, src);

	return len;
}