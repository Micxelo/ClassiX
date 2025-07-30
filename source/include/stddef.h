/*
	include/stddef.h
*/

#ifndef STDDEF_H
#define STDDEF_H

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef NULL
	#define NULL	((void *) 0)
#endif

typedef unsigned long size_t;

#ifdef __cplusplus
	}
#endif

#endif
