/*
	include/stdarg.h
*/

#ifndef _STDARG_H_
#define _STDARG_H_

#ifdef __cplusplus
	extern "C" {
#endif

typedef __builtin_va_list va_list;

#if defined __STDC_VERSION__ && __STDC_VERSION__ > 201710L
	#define va_start(v, ...)	__builtin_va_start(v, 0)
#else
	#define va_start(v,l)		__builtin_va_start(v,l)
#endif

#define va_end(v)			__builtin_va_end(v)
#define va_copy(d,s)		__builtin_va_copy(d,s)
#define va_arg				__builtin_va_arg

#ifdef __cplusplus
	}
#endif

#endif
