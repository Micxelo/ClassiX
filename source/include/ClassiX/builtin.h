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

extern const uint8_t builtin_terminus12n[];
extern const uint8_t builtin_terminus16n[];
extern const uint8_t builtin_terminus16b[];

#ifdef __cplusplus
	}
#endif

#endif