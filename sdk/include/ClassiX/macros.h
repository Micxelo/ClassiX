/*
	include/ClassiX/macros.h
*/

#ifndef _CLASSIX_MACROS_H_
#define _CLASSIX_MACROS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdint.h>

#define LOW8(u16) 							(uint8_t) ((u16) & 0x00ff)
#define HIGH8(u16) 							(uint8_t) (((u16) & 0xff00) >> 8)
#define LOW16(u32) 							(uint16_t) ((u32) & 0x0000ffff)
#define HIGH16(u32) 						(uint16_t) (((u32) & 0xffff0000) >> 16)

#ifdef __cplusplus
	}
#endif

#endif
