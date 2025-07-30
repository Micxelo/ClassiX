/*
	include/ClassiX/typedef.h
*/

#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef NULL
	#define NULL					((void *) 0)
#endif

typedef void * handle;

typedef uint32_t color_t;

#ifdef __cplusplus
	}
#endif

#endif
