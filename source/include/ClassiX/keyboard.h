/*
	include/ClassiX/keyboard.h
*/

#ifndef _CLASSIX_KEYBOARD_H_
#define _CLASSIX_KEYBOARD_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>

void init_keyboard(FIFO *fifo, int32_t data0);
void wait_kbc_sendready(void);

#define EXPANDED_KEY_PREFIX					(0xe0)

extern const uint8_t keymap0[];
extern const uint8_t keymap1[];

#ifdef __cplusplus
	}
#endif

#endif
