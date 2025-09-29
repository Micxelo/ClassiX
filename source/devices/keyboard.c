/*
	devices/keyboard.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/fifo.h>
#include <ClassiX/keyboard.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

#define PORT_KEYBOARD_DATA					(0x0060)
#define PORT_KEYBOARD_COMMAND				(0x0064)
#define PORT_KEYSTA							(0x0064)
#define KEYSTA_SEND_NOTREADY				(0x02)
#define KEYCMD_WRITE_MODE					(0x60)
#define KBC_MODE							(0x47)

static uint32_t keydata0;
static FIFO *keyboard_fifo;

/*	@brief 初始化键盘。
	@param fifo FIFO 缓冲区指针
	@param data0 键盘数据偏移量
*/
void init_keyboard(FIFO *fifo, int data0)
{
	/* 将 FIFO 缓冲区的内容保存到全局变量里 */
	keyboard_fifo = fifo;
	keydata0 = data0;

	/* 注册 IRQ */
	extern void asm_isr_keyboard(void);
	idt_set_gate(INT_NUM_KEYBOARD, (uint32_t) asm_isr_keyboard, 0x08, AR_INTGATE32);

	/* 键盘控制电路初始化 */
	wait_kbc_sendready();
	out8(PORT_KEYBOARD_COMMAND, KEYCMD_WRITE_MODE);
	wait_kbc_sendready();
	out8(PORT_KEYBOARD_DATA, KBC_MODE);
	
	debug("Keyboard initialized.\n");
}

/*
	@brief 等待键盘控制器发送就绪。
*/
void wait_kbc_sendready(void)
{
	for (;;)
		if ((in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
			break;
	return;
}

void isr_keyboard(ISR_PARAMS params)
{
	uint32_t data;
	out8(PIC0_OCW2, 0x20); /* 主 PIC EOI */
	data = in8(PORT_KEYBOARD_DATA);
	fifo_push(keyboard_fifo, data + keydata0);
	debug("Keyboard data: 0x%02x.\n", data);
	return;
}

/* 键盘映射表 */
const uint8_t keymap0[] = {
	0,   0,   '1', '2',  '3', '4', '5', '6', '7',  '8', '9', '0',  '-',  '=',  0x08, 0,
	'Q', 'W', 'E', 'R',  'T', 'Y', 'U', 'I', 'O',  'P', '[', ']',  0x0a, 0,    'A', 'S',
	'D', 'F', 'G', 'H',  'J', 'K', 'L', ';', '\'', '`',   0,  '\\', 'Z', 'X',  'C', 'V',
	'B', 'N', 'M', ',',  '.', '/', 0,   '*', 0,    ' ', 0,   0,    0,    0,    0,   0,
	0,   0,   0,   0,    0,   0,   0,   '7', '8',  '9', '-', '4',  '5',  '6',  '+', '1',
	'2', '3', '0', '.',  0,   0,   0,   0,   0,    0,   0,   0,    0,    0,    0,   0,
	0,   0,   0,   0,    0,   0,   0,   0,   0,    0,   0,   0,    0,    0,    0,   0,
	0,   0,   0,   0x5c, 0,   0,   0,   0,   0,    0,   0,   0,    0,    0x5c, 0,   0
};

/* 按下 SHIFT 时的键盘映射表 */
const uint8_t keymap1[] = {
	0,   0,   '!', '@', '#', '$', '%', '^', '&',  '*', '(', ')', '_',  '+', 0x08, 0,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O',  'P', '{', '}', 0x0a, 0,   'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,   '|', 'Z',  'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,    ' ', 0,   0,   0,    0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   '7', '8',  '9', '-', '4', '5',  '6', '+', '1',
	'2', '3', '0', '.', 0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
	0,   0,   0,   '_', 0,   0,   0,   0,   0,    0,   0,   0,   0,    '|', 0,   0
};
