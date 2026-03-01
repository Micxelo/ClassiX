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

	static inline void hexdump(const void *ptr, size_t len) {
		const uint8_t *p = (const uint8_t *) ptr;

		for (size_t i = 0; i < len; i += 16) {
			/* 地址 */
			debug("0x%08x  ", (uint32_t) ((uintptr_t) p + i));

			/* HEX */
			for (size_t j = 0; j < 16; j++)
				if (i + j < len)
					debug("%02x ", p[i + j]);
				else
					debug("   ");

			debug(" ");

			/* ASCII */
			for (size_t j = 0; j < 16; j++)
				if (i + j < len) {
					uint8_t c = p[i + j];
					if (c >= 32 && c <= 126)
						debug("%c", c);
					else
						debug(".");
				} else {
					debug(" ");
				}

			debug("\n");
		}
	}
#else
	#define debug(fmt, ...)					((void) 0)
	#define assert(condition, info)			((void) 0)
	#define hexdump(ptr, len)				((void) 0)
#endif

#ifdef __cplusplus
	}
#endif

#endif
