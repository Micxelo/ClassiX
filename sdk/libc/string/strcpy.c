/*
	libc/string/strcpy.c
*/

#include <string.h>

char *strcpy(char *restrict dest, const char *restrict src)
{
	char *tmp = dest;
	while ((*dest++ = *src++) != '\0') { }
	return tmp;
}