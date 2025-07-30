/*
	devices/keyboard.c
*/

#include <ClassiX/typedef.h>
#include <ClassiX/io.h>
#include <ClassiX/int.h>
#include <ClassiX/keyboard.h>

void inthandler_21(uint32_t *esp)
{
	int data;
	out8(PIC0_OCW2, 0x61);	/* IRQ1 已受理" */
	data = in8(PORT_KEYDATA);
	return;
}
