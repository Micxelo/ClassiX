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
static bool mouse_has_wheel = false;

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

	/* 注册 IRQ */
	extern void asm_isr_mouse(void);
	idt_set_gate(INT_NUM_MOUSE, (uint32_t) asm_isr_mouse, 0x08, AR_INTGATE32);

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

	/* 发送魔术序列：200, 100, 80 */
	mouse_write(0xf3);
	mouse_write(200);
	mouse_write(0xf3);
	mouse_write(100);
	mouse_write(0xf3);
	mouse_write(80);

	mouse_write(0xf2);						/* 获取鼠标 ID */
	uint8_t ack = mouse_read();				/* 等待 ACK */
	uint8_t mouse_id = mouse_read();

	if (ack == 0xfa && mouse_id == 0x03) {
		/* 鼠标支持滚轮 */
		mouse_has_wheel = 1;
		debug("Wheel mouse detected (ID: 0x%x).\n", mouse_id);

		/* 设置 4 字节数据包模式 */
		/* 发送魔术序列：200, 100, 80 */
		mouse_write(0xf3);
		mouse_write(200);
		mouse_write(0xf3);
		mouse_write(100);
		mouse_write(0xf3);
		mouse_write(80);

		/* 再次获取设备 ID 确认 */
		mouse_write(0xf2);
		ack = mouse_read();					/* 等待 ACK */
		mouse_id = mouse_read();

		if (ack == 0xfa && mouse_id == 0x03) {
			debug("Wheel mode enabled successfully\n");
		} else {
			debug("Failed to enable wheel mode (ID: 0x%x)\n", mouse_id);
			mouse_has_wheel = 0;
		}
	} else {
		/* 标准 PS/2 鼠标 */
		mouse_has_wheel = 0;
		debug("Standard mouse detected (ID: 0x%x).\n", mouse_id);
	}

	mouse_write(0xf4);						/* 启用数据报告 */
	mouse_read();							/* 等待 ACK */
	debug("Mouse initialized %s.\n", mouse_has_wheel ? "with wheel support" : "without wheel support");
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

		if (mouse_has_wheel) {
			mouse_data->phase = 3;
		} else {
			/* 完整数据包已接收 */
			mouse_data->dz = 0; /* 无滚轮数据 */
			mouse_data->phase = 0; /* 重置阶段 */
			debug("Mouse data: dX=%d, dY=%d, Buttons=%08b.\n", 
				mouse_data->dx, mouse_data->dy, mouse_data->button);
			return 1;
		}
		break;
	case 3:
		/* Bit4：滚轮 */
		mouse_data->dz = (int8_t) ((data & 0x0f) | ((data & 0x08) ? 0xf0 : 0x00));
		/* 按钮 4、5 */
		if (data & 0b00010000) mouse_data->button |= 0b00010000;
		if (data & 0b00100000) mouse_data->button |= 0b00100000;
		mouse_data->phase = 0; /* 重置阶段 */
			debug("Mouse data: dX=%d, dY=%d, dz=%d, Buttons=%08b.\n", 
				mouse_data->dx, mouse_data->dy, mouse_data->dz, mouse_data->button);
		return 1;
	default:
		debug("Unknown mouse phase: %d.\n", mouse_data->phase);
		mouse_data->phase = 0; /* 重置阶段 */
		return 0;
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

inline bool mouse_has_wheel_support(void)
{
	return mouse_has_wheel;
}
