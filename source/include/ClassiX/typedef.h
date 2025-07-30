/*
	include/ClassiX/typedef.h
*/

#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef NULL
	#define NULL					((void *) 0)
#endif

typedef void * HANDLE;

typedef uint32_t COLOR;

#ifdef __cplusplus
	}
#endif

#endif
