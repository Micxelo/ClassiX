/*
	include/ClassiX/debug.h
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/serial.h>
#include <ClassiX/typedef.h>

#define DEBUG

#ifdef DEBUG
	#define debug(fmt, ...)				uart_printf(fmt, ##__VA_ARGS__)
#else
	#define debug(fmt, ...)				((void) 0)
#endif

#ifdef __cplusplus
	}
#endif

#endif
