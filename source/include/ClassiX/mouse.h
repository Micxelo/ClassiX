/*
	include/ClassiX/mouse.h
*/

#ifndef _MOUSE_H_
#define _MOUSE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>

#define INT_NUM_MOUSE						0x2c

#define MOUSE_LBUTTON						0b00000001
#define MOUSE_RBUTTON						0b00000010
#define MOUSE_MBUTTON						0b00000100

typedef struct {
	uint8_t phase;
	int8_t dx, dy;
	uint8_t button;
	int8_t dz; /* 鼠标滚轮 */ 
} MOUSE_DATA;

void init_mouse(FIFO *fifo, int data0);
int mouse_decoder(MOUSE_DATA *mouse_data, uint8_t data);
void isr_mouse(isr_params_t params);

#ifdef __cplusplus
	}
#endif

#endif
