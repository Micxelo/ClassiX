/*
	include/stddef.h
*/

#ifndef _STDDEF_H_
#define _STDDEF_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdint.h>

#ifndef NULL
	#define NULL							((void *) 0)
#endif

typedef unsigned long size_t;
typedef int64_t ssize_t;

#ifdef __cplusplus
	}
#endif

#endif
