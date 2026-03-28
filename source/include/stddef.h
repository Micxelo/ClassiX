/*
	include/stddef.h
*/

#ifndef _STDDEF_H_
#define _STDDEF_H_

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef NULL
	#define NULL							((void *) 0)
#endif

#ifndef offsetof
	#define offsetof(type, member)			((size_t) &((type *) 0)->member)
#endif

#ifdef __PTRDIFF_TYPE__
	typedef __PTRDIFF_TYPE__ ptrdiff_t;
#endif

#ifdef __SIZE_TYPE__
	typedef __SIZE_TYPE__ size_t;
#else
	typedef unsigned int size_t;
#endif
typedef long ssize_t;

#ifdef __cplusplus
	}
#endif

#endif
