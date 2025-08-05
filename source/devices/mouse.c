/*
	devices/mouse.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/keyboard.h>
#include <ClassiX/mouse.h>
#include <ClassiX/typedef.h>

#define PORT_MOUSE_DATA						0x60
#define PORT_MOUSE_STATUS					0x64
#define PORT_MOUSE_COMMAND					0x64
#define KEYCMD_SENDTO_MOUSE					0xd4
#define MOUSECMD_ENABLE						0xf4

static uint32_t mousedata0;
static FIFO *mouse_fifo;

/*
	@brief 初始化鼠标。
	@param fifo 鼠标数据的 FIFO
	@param data0 鼠标数据的偏移量
*/
void init_mouse(FIFO *fifo, int data0)
{
	mouse_fifo = fifo;
	mousedata0 = data0;

	/* 注册 IRQ */
	extern void asm_isr_mouse(void);
	idt_set_gate(INT_NUM_MOUSE, (uint32_t) asm_isr_mouse, 0x08, AR_INTGATE32);

	wait_kbc_sendready();
	out8(PORT_MOUSE_COMMAND, KEYCMD_SENDTO_MOUSE);
	wait_kbc_sendready();
	out8(PORT_MOUSE_DATA, MOUSECMD_ENABLE);

	debug("Mouse initialized.\n");
}

/*
	@brief 处理鼠标数据包。
	@param mouse_data 鼠标数据结构体指针
	@param data 鼠标数据字节
	@return 如果接收到完整的数据包，返回 1；否则返回 0。
*/
int mouse_decoder(MOUSE_DATA *mouse_data, uint8_t data)
{
	if (data == 0xfa) {
		/* 初始化确认包，重置状态 */
		mouse_data->phase = 0;
		return 0; /* 无数据包 */
	}
	/* 处理鼠标数据包 */
	switch (mouse_data->phase) {
		case 0:
			/* Bit1：按钮状态和标志位 */
			if ((data & 0xc8) == 0x08) {
				mouse_data->button = data; /* 按钮状态 */
				mouse_data->phase = 1;
			} else {
				/* 无效数据，重置状态 */
				mouse_data->phase = 0;
			}
			break;

		case 1:
			/* Bit2：X 位移 */
			mouse_data->dx = (int8_t) data;
			mouse_data->phase = 2;
			break;

		case 2:
			/* Bit3：Y 位移 */
			mouse_data->dy = (int8_t) data;

			if (mouse_data->button & 0x10) mouse_data->dx |= 0xffffff00;
			if (mouse_data->button & 0x20) mouse_data->dy |= 0xffffff00;
			mouse_data->button &= 0b00000111;
			mouse_data->dy = -mouse_data->dy; /* Y 轴反转 */

			/* 完整数据包已接收 */
			mouse_data->dz = 0; /* 无滚轮数据 */
			mouse_data->phase = 0; /* 重置阶段 */
			debug("Mouse data: dX=%d, dY=%d, Buttons=%03b.\n",
				mouse_data->dx, mouse_data->dy, mouse_data->button);
			return 1;
			break;

		default:
			debug("Unknown mouse phase: %d.\n", mouse_data->phase);
			mouse_data->phase = 0; /* 重置阶段 */
			return 0;
	}
	return 0;
}

void isr_mouse(isr_params_t params)
{
	out8(PIC1_OCW2, 0x20); /* 从 PIC EOI */
	out8(PIC0_OCW2, 0x20); /* 主 PIC EOI */

	uint32_t data = in8(PORT_MOUSE_DATA);
	fifo_push(mouse_fifo, data + mousedata0);
	return;
}
