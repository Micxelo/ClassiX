/*
	include/ClassiX/serial.h
*/

#ifndef _CLASSIX_SERIAL_H_
#define _CLASSIX_SERIAL_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#include <stdarg.h>

void uart_init(void);
void uart_putchar(char c);
void uart_puts(const char *str);
int uart_printf(const char *format, ...);

#ifdef __cplusplus
	}
#endif

#endif
