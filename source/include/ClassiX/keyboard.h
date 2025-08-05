/*
	include/ClassiX/keyboard.h
*/

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>

#define INT_NUM_KEYBOARD					0x21

void init_keyboard(FIFO *fifo, int data0);
void wait_kbc_sendready(void);
void isr_keyboard(isr_params_t params);

#ifdef __cplusplus
	}
#endif

#endif
