/*
	include/ClassiX/keyboard.h
*/

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define PORT_KEYDATA						0x0060
#define PORT_KEYCMD							0x0064

void isr_keyboard(uint32_t *esp);

#ifdef __cplusplus
	}
#endif

#endif
