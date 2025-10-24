/*
	include/stdio.h
*/

#ifndef _STDIO_H_
#define _STDIO_H_

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef NULL
	#define NULL					((void *) 0)
#endif

#include <stdarg.h>

int32_t vsprintf(char *buf, char const *fmt, va_list va);
int32_t vsnprintf(char *buf, int32_t count, char const *fmt, va_list va);
int32_t sprintf(char *buf, char const *fmt, ...) __attribute__((format(printf, 2, 3)));
int32_t snprintf(char *buf, int32_t count, char const *fmt, ...) __attribute__((format(printf, 3, 4)));

#ifdef __cplusplus
	}
#endif

#endif
