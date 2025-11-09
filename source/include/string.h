/*
	include/string.h
*/

#ifndef _STRING_H_
#define _STRING_H_

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef NULL
	#define NULL					((void *) 0)
#endif

#include <stddef.h>
#include <stdint.h>

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t count);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t count);
int32_t strcmp(const char *s1, const char *s2);
int32_t strncmp(const char *s1, const char *s2, size_t count);
char *strchr(const char *s, char c);
char *strrchr(const char *s, char c);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *accept);
char *strpbrk(const char *s, const char *accept);
char *strstr(const char *s1, const char *s2);
size_t strlen(const char *s);
char *strtok(char *s, const char *delim);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int32_t memcmp(const void *s1, const void *s2, size_t n);
void *memchr(void *s, uint8_t c, size_t n);
void *memset(void *s, uint8_t c, size_t n);

#ifdef __cplusplus
	}
#endif

#endif
