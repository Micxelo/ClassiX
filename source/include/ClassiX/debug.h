/*
	include/ClassiX/debug.h
*/

#ifndef _CLASSIX_DEBUG_H_
#define _CLASSIX_DEBUG_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/serial.h>
#include <ClassiX/typedef.h>

#define DEBUG

#ifdef DEBUG
	#define debug(fmt, ...)					uart_printf(fmt, ##__VA_ARGS__)
	#define assert(condition)			do {							\
		if(!(condition)) {												\
			debug("[Assert] %s:%d\n", __FILE__, __LINE__);				\
			asm volatile ("    cli\n""1:  hlt\n""    jmp 1b");			\
		}																\
	} while(0)
#else
	#define debug(fmt, ...)					((void) 0)
	#define assert(condition, info)			((void) 0)
#endif

#ifdef __cplusplus
	}
#endif

#endif
