/*
	include/ClassiX/programs.h
*/

#ifndef _CLASSIX_PROGRAMS_H_
#define _CLASSIX_PROGRAMS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

enum {
	SRV_SUCCESS = 0,
	SRV_INVALID_PARAM,
	SRV_NOT_FOUND,
	SRV_MEMORY_ALLOC,
	SRV_READ_FAIL,
	SRV_INVALID_FORMAT
};

int32_t program_exec(int32_t argc, char **argv);

#ifdef __cplusplus
	}
#endif

#endif