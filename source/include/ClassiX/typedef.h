/*
	include/ClassiX/typedef.h
*/

#ifndef _CLASSIX_TYPEDEF_H_
#define _CLASSIX_TYPEDEF_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef NULL
	#define NULL							((void *) 0)
#endif

#define ARRAY_SIZE(arr)						(sizeof(arr) / sizeof((arr)[0]))

typedef void * HANDLE;

#ifdef __cplusplus
	}
#endif

#endif
