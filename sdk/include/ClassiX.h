/*
	include/ClassiX.h
*/

#ifndef _CLASSIX_H_
#define _CLASSIX_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "ClassiX/events.h"
#include "ClassiX/macros.h"
#include "ClassiX/palette.h"
#include "ClassiX/typedef.h"
#include "ClassiX/window.h"

#include <stdint.h>

extern void __attribute__((noreturn)) cx_exit_process(int32_t status);
extern void cx_debug_print(const char* str);

#ifdef __cplusplus
	}
#endif

#endif