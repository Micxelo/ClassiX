/*
	include/string.h
*/

#ifndef _STRING_H_
#define _STRING_H_

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef NULL
	#define NULL							((void *) 0)
#endif

#ifdef __SIZE_TYPE__
	typedef __SIZE_TYPE__ size_t;
#else
	typedef unsigned int size_t;
#endif

char *strcpy(char *restrict dest, const char *restrict src);
char *strncpy(char *restrict dest, const char *restrict src, size_t count);
char *strcat(char *restrict dest, const char *restrict src);
char *strncat(char *restrict dest, const char *restrict src, size_t count);
size_t strxfrm(char *restrict dest, const char *restrict src, size_t count);

size_t strlen(const char *s);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, size_t count);
int strcoll(const char *cs, const char *ct);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *cs, const char *ct);
char *strstr(const char *s1, const char *s2);
char *strtok(char *restrict s, const char *restrict delim);

void *memchr(void *s, int c, size_t n);
int memcmp(const void *cs, const void *ct, size_t count);
void *memset(void *s, int c, size_t count);
void *memcpy(void *restrict dest, const void *restrict src, size_t count);
void *memmove(void *dest, const void *src, size_t count);

char *strerror(int errnum);

#ifdef __cplusplus
	}
#endif

#endif
