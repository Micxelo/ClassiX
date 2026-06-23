/*
	libc/string/strcoll.c
*/

#include <string.h>

int strcoll(const char *cs, const char *ct)
{
	return strcmp(cs, ct);
}