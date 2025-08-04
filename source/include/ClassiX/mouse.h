/*
	include/ClassiX/mouse.h
*/

#ifndef _MOUSE_H_
#define _MOUSE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define PORT_MOUSE_DATA						0x60
#define PORT_MOUSE_STATUS					0x64
#define PORT_MOUSE_COMMAND					0x64

typedef struct {
	uint8_t phase;
	uint8_t buf[3];
	int8_t dx, dy;
	uint8_t button;
} MOUSE_DATA;

void init_mouse(FIFO *fifo, int data0);
int mouse_decoder(MOUSE_DATA *mouse_data, uint8_t data);
void isr_mouse(uint32_t *esp);

#ifdef __cplusplus
	}
#endif

#endif
