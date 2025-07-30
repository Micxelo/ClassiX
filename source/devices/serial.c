/*
	devices/serial.c
*/

#include <ClassiX/serial.h>
#include <ClassiX/io.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#define COM1_BASE							0x3f8

/* 寄存器偏移量 */
enum UART_REGISTERS {
	THR = 0,	/* 发送保持寄存器 (写) */
	LCR = 3,	/* 线路控制寄存器 */
	LSR = 5,	/* 线路状态寄存器 */
	DLL = 0,	/* 除数锁存低字节 (DLAB = 1) */
	DLH = 1		/* 除数锁存高字节 (DLAB = 1) */
};

void uart_init(void)
{
	out8(COM1_BASE + 1, 0x00);	/* 禁用串口中断 */

	out8(COM1_BASE + LCR, 0x80);	/* 启用 DLAB */
	out8(COM1_BASE + DLL, 0x01);	/* 115200 bps */
	out8(COM1_BASE + DLH, 0x00);

	/* 设置帧格式: 8 位数据, 无校验, 1 位停止位 */
	out8(COM1_BASE + LCR, 0x03);

	out8(COM1_BASE + 2, 0xC7);	/* 启用 FIFO */

	/* 启用 OUT2 (使能串口工作) */
	out8(COM1_BASE + 4, 0x08); // MCR 寄存器
}

// 发送单个字符 (轮询方式)
void uart_putchar(char c)
{
	/* 等待发送缓冲区为空 (检查 LSR 第 5 位) */
	while ((in8(COM1_BASE + LSR) & 0x20) == 0) { } /* 忙等待 */

	out8(COM1_BASE + THR, c);

	if (c == '\n') {
		uart_putchar('\r');
	}
}

// 发送字符串
void uart_puts(const char *str)
{
	while (*str)
		uart_putchar(*str++);
}

int uart_printf(const char *format, ...)
{
	const size_t max_length = 1024;
	char buf[max_length];

	int result;
	va_list va;
	va_start(va, format);

	result = vsnprintf(buf, max_length, format, va);
	va_end(va);

	uart_puts(buf);
	return result;
}
