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

typedef unsigned long size_t;

#ifdef __cplusplus
	}
#endif

#endif
