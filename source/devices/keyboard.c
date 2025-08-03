/*
	devices/keyboard.c
*/

#include <ClassiX/keyboard.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

void isr_keyboard(uint32_t *esp)
{
	int data;
	out8(PIC0_OCW2, 0x20);
	data = in8(PORT_KEYDATA);
	#include <ClassiX/debug.h>
	debug("%02x\n", data);
	return;
}
