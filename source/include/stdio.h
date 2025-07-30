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

int vsprintf(char *buf, char const *fmt, va_list va);
int vsnprintf(char *buf, int count, char const *fmt, va_list va);
int sprintf(char *buf, char const *fmt, ...) __attribute__((format(printf, 2, 3)));
int snprintf(char *buf, int count, char const *fmt, ...) __attribute__((format(printf, 3, 4)));

#ifdef __cplusplus
	}
#endif

#endif
