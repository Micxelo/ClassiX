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

#ifndef __cplusplus
	#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
		#define static_assert _Static_assert
	#else
		#define static_assert(cond, msg)
	#endif
#endif

#ifdef __cplusplus
	}
#endif

#endif
