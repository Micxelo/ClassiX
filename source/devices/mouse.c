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

#define KEYCMD_SENDTO_MOUSE					0xd4
#define MOUSECMD_ENABLE						0xf4

static uint32_t mousedata0;
static FIFO *mouse_fifo;

static void mouse_wait_write(void)
{
	for (;;)
		if ((in8(PORT_MOUSE_STATUS) & 0x02) == 0)
			break;
}

static void mouse_wait_read(void)
{
	for (;;)
		if ((in8(PORT_MOUSE_STATUS) & 0x01) == 1)
			break;
}

static void mouse_write(uint8_t data)
{
	mouse_wait_write();
	out8(PORT_MOUSE_COMMAND, KEYCMD_SENDTO_MOUSE);
	mouse_wait_write();
	out8(PORT_MOUSE_DATA, data);
}

static uint8_t mouse_read(void)
{
	mouse_wait_read();
	return in8(PORT_MOUSE_DATA);
}

/*
	@brief 初始化鼠标。
	@param fifo 鼠标数据的 FIFO
	@param data0 鼠标数据的偏移量
*/
void init_mouse(FIFO *fifo, int data0)
{
	mouse_fifo = fifo;
	mousedata0 = data0;

	uint8_t status;

	/* 启用鼠标 */
	mouse_wait_write();
	out8(PORT_MOUSE_COMMAND, 0xa8);

	/* 启用中断 */
	mouse_wait_write();
	out8(PORT_MOUSE_COMMAND, 0x20);			/* 获取当前配置 */
	mouse_wait_read();
	status = in8(PORT_MOUSE_DATA) | 0x02;	/* 设置鼠标中断使能位 */
	mouse_wait_write();
	out8(PORT_MOUSE_COMMAND, 0x60);			/* 设置新配置 */
	mouse_wait_write();
	out8(PORT_MOUSE_DATA, status);

	
	mouse_write(0xf6);						/* 设置默认参数 */
	mouse_read();							/* 等待 ACK */

	mouse_write(0xF4);						/* 启用数据报告 */
	mouse_read();							/* 等待 ACK */

	debug("Mouse initialized\n");
}

/*
	@brief 处理鼠标数据包。
	@param mouse_data 鼠标数据结构体指针
	@param data 鼠标数据字节
	@return 如果接收到完整的数据包，返回 1；否则返回 0。
*/
int mouse_decoder(MOUSE_DATA *mouse_data, uint8_t data)
{
	/* 处理鼠标数据包 */
	if (mouse_data->phase == 0) {
		/* 等待鼠标 0xfa 阶段 */
		if (data == 0xfa) {
			mouse_data->phase = 1;
		}
	}
	if (mouse_data->phase == 1) {
		/* 等待鼠标 bit1 阶段 */
		if ((data & 0xc8) == 0x08) {
			mouse_data->buf[0] = data;
			mouse_data->phase = 2;
		}
	}
	if (mouse_data->phase == 2) {
		/* 等待鼠标 bit2 阶段 */
		mouse_data->buf[1] = data;
		mouse_data->phase = 3;
	}
	if (mouse_data->phase == 3) {
		/* 等待鼠标 bit3 阶段 */
		mouse_data->buf[2] = data;
		mouse_data->phase = 1;
		mouse_data->button = mouse_data->buf[0] & 0x07;
		mouse_data->dx = mouse_data->buf[1];
		mouse_data->dy = mouse_data->buf[2];
		if ((mouse_data->buf[0] & 0x10) != 0) mouse_data->dx |= 0xffffff00;
		if ((mouse_data->buf[0] & 0x20) != 0) mouse_data->dy |= 0xffffff00;
		mouse_data->dy = - mouse_data->dy; /* 反转 Y 轴 */

		
		/* 完整数据包已接收 */
		debug("Mouse data: dX=%d, dY=%d, Buttons=0x%x.\n", 
			mouse_data->dx, mouse_data->dy, mouse_data->button);
		return 1;
	}
	return 0;
}

void isr_mouse(uint32_t *esp)
{
    out8(PIC1_OCW2, 0x20); /* 从 PIC EOI */
    out8(PIC0_OCW2, 0x20); /* 主 PIC EOI */

	uint32_t data = in8(PORT_MOUSE_DATA);
	fifo_push(mouse_fifo, data + mousedata0);
	return;
}
