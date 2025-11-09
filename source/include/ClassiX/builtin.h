/*
	include/ClassiX/builtin.h
*/

#ifndef _CLASSIX_BUILTIN_H_
#define _CLASSIX_BUILTIN_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define MAJOR
#define MINOR
#define PATCH

#define CURSOR_WIDTH						16
#define CURSOR_HEIGHT						16
extern const uint32_t builtin_cursor_arrow[];

#ifdef __cplusplus
	}
#endif

#endif