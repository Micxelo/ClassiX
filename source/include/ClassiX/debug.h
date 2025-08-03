/*
	include/ClassiX/debug.h
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/io.h>
#include <ClassiX/serial.h>
#include <ClassiX/typedef.h>

#define DEBUG

#ifdef DEBUG
	#define debug(fmt, ...)					uart_printf(fmt, ##__VA_ARGS__)
	#define assert(condition, info)			do {						\
		if(!(condition)) {												\
			debug("[Assert] %s:%d  %s\n", __FILE__, __LINE__, info);	\
			cli();														\
			for (;;) hlt();												\
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
