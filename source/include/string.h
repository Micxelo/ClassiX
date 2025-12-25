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

#include <stddef.h>
#include <stdint.h>

static inline char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;
	while ((*dest++ = *src++) != '\0') { }
	return tmp;
}

static inline char *strncpy(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	while (count) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		count--;
	}

	return dest;
}

static inline char *strcat(char *dest, const char *src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0') { }

	return tmp;
}

static inline char *strncat(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	if (count) {
		while (*dest)
			dest++;
		while ((*dest++ = *src++) != 0) {
			if (--count == 0) {
				*dest = '\0';
				break;
			}
		}
	}

	return tmp;
}

static inline int32_t strcmp(const char *cs, const char *ct)
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

static inline int32_t strncmp(const char *cs, const char *ct, size_t count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
		count--;
	}

	return 0;
}

static inline char *strchr(const char *s, char c)
{
	for (; *s != c; ++s)
		if (*s == '\0')
			return NULL;

	return (char *) s;
}

static inline char *strrchr(const char *s, char c)
{
	const char *last = NULL;

	do {
		if (*s == c)
			last = s;
	} while (*s++);

	return (char *) last;
}

static inline size_t strlen(const char *s)
{
	const char *sc;
	for (sc = s; *sc != '\0'; ++sc) { }
	return sc - s;
}

static inline int32_t memcmp(const void *cs, const void *ct, size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;

	return res;
}

static inline size_t strspn(const char *s, const char *accept)
{
	const char *p;

	for (p = s; *p != '\0'; ++p)
		if (!strchr(accept, *p))
			break;

	return p - s;
}

static inline size_t strcspn(const char *s, const char *reject)
{
	const char *p;

	for (p = s; *p != '\0'; ++p)
		if (strchr(reject, *p))
			break;

	return p - s;
}

static inline char *strpbrk(const char *cs, const char *ct)
{
	const char *sc;

	for (sc = cs; *sc != '\0'; ++sc)
		if (strchr(ct, *sc))
			return (char *)sc;

	return NULL;
}

static inline char *strstr(const char *s1, const char *s2)
{
	size_t l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1, s2, l2))
			return (char *) s1;
		s1++;
	}

	return NULL;
}

static inline void *memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;

	while (count--)
		*tmp++ = *s++;

	return dest;
}

static inline void *memmove(void *dest, const void *src, size_t count)
{
	char *tmp;
	const char *s;

	if (dest <= src) {
		tmp = dest;
		s = src;
		while (count--)
			*tmp++ = *s++;
	} else {
		tmp = dest;
		tmp += count;
		s = src;
		s += count;
		while (count--)
			*--tmp = *--s;
	}

	return dest;
}

static inline void *memchr(void *s, uint8_t c, size_t n)
{
	const unsigned char *p = s;

	while (n-- != 0)
		if (c == *p++)
			return (void *) (p - 1);
		
	return NULL;
}

static inline void *memset(void *s, uint8_t c, size_t count)
{
	char *xs = s;

	while (count--)
		*xs++ = c;

	return s;
}


#ifdef __cplusplus
	}
#endif

#endif
