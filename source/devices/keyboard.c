/*
	devices/keyboard.c
*/

#include <ClassiX/keyboard.h>
#include <ClassiX/int.h>
#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

void inthandler_21(uint32_t *esp)
{
	int data;
	out8(PIC0_OCW2, 0x61);	/* IRQ1 已受理" */
	data = in8(PORT_KEYDATA);
	return;
}
