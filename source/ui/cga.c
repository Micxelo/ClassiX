/*
	ui/cga.c
*/

#include <ClassiX/cga.h>
#include <ClassiX/io.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define CGA_SIZE						(CGA_COLUMNS * CGA_ROWS)
#define CGA_ENTRY(c, color)				((uint16_t) (c) | (uint16_t) (color) << 8)

static uint16_t cga_cursor_x = 0;
static uint16_t cga_cursor_y = 0;
static uint16_t cga_color = CGA_LIGHTGRAY | CGA_BLACK << 4;		/* 黑底白字 */
static uint16_t *buffer = (uint16_t *) CGA_BUF_ADDR;

/*
	@brief 设置 CGA 80*25 终端输出字符位置。
*/
inline void set_cga_cursor_pos(uint16_t x, uint16_t y)
{
	if (x >= CGA_COLUMNS || y >= CGA_ROWS) {
		return;
	}
	cga_cursor_x = x;
	cga_cursor_y = y;
}

/*
	@brief 获取 CGA 80*25 终端输出字符位置。
	@return x + y * CGA_COLUMNS
*/
uint16_t get_cga_cursor_pos(void)
{
	return cga_cursor_x + cga_cursor_y * CGA_COLUMNS;
}


/*
	@brief 设置 CGA 80*25 终端输出字符颜色。
*/
inline void set_cga_color(uint16_t color)
{
	cga_color = color;
}

/*
	@brief 清空 CGA 80*25 终端缓冲区。
*/
void cga_clear(void)
{
	cga_cursor_x = 0;
	cga_cursor_y = 0;
	for (size_t i = 0; i < CGA_SIZE; i++) {
		buffer[i] = CGA_ENTRY(' ', cga_color);
	}
}

static void cga_backspace(void)
{
	if (cga_cursor_x > 0) {
		cga_cursor_x--;
	} else if (cga_cursor_y > 0) {
		cga_cursor_y--;
		cga_cursor_x = CGA_COLUMNS - 1;
	}
	const size_t idx = cga_cursor_y * CGA_COLUMNS + cga_cursor_x;
	buffer[idx] = CGA_ENTRY(' ', cga_color);
}

static void cga_newline(void)
{
	if (cga_cursor_y < CGA_ROWS - 1) {
		cga_cursor_y++;
	} else {
		/* 滚屏 */
		memmove(buffer, buffer + CGA_COLUMNS, (CGA_ROWS - 1) * CGA_COLUMNS * sizeof(uint16_t));
		for (uint16_t x = 0; x < CGA_COLUMNS; x++) {
			const size_t idx = (CGA_ROWS - 1) * CGA_COLUMNS + x;
			buffer[idx] = CGA_ENTRY(' ', cga_color);
		}
	}
	cga_cursor_x = 0;
}

static void cga_tab(void)
{
	const uint16_t tab_size = 4;	/* 仅支持 2、4、8 ... （16 进制下的整数） */
	if (cga_cursor_x > CGA_COLUMNS - tab_size - 1) {
		cga_newline();
	} else {
		do {
			/* 不被 tab_size 整除 */
			const size_t idx = cga_cursor_x + cga_cursor_y * CGA_COLUMNS;
			buffer[idx] = CGA_ENTRY(' ', cga_color);
			cga_cursor_x++;
		} while (cga_cursor_x & (tab_size - 1));
	}
}

#define CGA_CURSOR_BASE_IDX 				0x3d4
#define CGA_CURSOR_BASE_DATA 				0x3d5

/*
	@brief 设置 CGA 80*25 终端光标位置。
*/
void cga_move_cursor(uint16_t x, uint16_t y)
{
	const size_t pos = x + y * CGA_COLUMNS;
	out8(CGA_CURSOR_BASE_IDX, 0x0f);
	out8(CGA_CURSOR_BASE_DATA, pos & 0xff);
	out8(CGA_CURSOR_BASE_IDX, 0x0e);
	out8(CGA_CURSOR_BASE_DATA, (pos >> 8) & 0xff);
}

/*
	@brief 隐藏 CGA 80*25 终端光标。
*/
void cga_hide_cursor(void)
{
	out8(CGA_CURSOR_BASE_IDX, 0x0a);
	out8(CGA_CURSOR_BASE_DATA, 0x20);
}

/*
	@brief 显示 CGA 80*25 终端光标位置。
*/
void cga_show_cursor(void)
{
	out8(CGA_CURSOR_BASE_IDX, 0x0a);
	out8(CGA_CURSOR_BASE_DATA, 0x0e);
	out8(CGA_CURSOR_BASE_IDX, 0x0b);
	out8(CGA_CURSOR_BASE_DATA, 0x0f);
}

/*
	@brief CGA 80*25 打印单个字符。
*/
void cga_putchar(char c)
{
	switch (c) {
		case '\b':
			cga_backspace();
			break;
		case '\r':
			cga_cursor_x = 0;
			break;
		case '\n':
			cga_newline();
			break;
		case '\t':
			cga_tab();
			break;
		default:
			const size_t idx = cga_cursor_x + cga_cursor_y * CGA_COLUMNS;
			buffer[idx] = CGA_ENTRY(c, cga_color);
			if (++cga_cursor_x >= CGA_COLUMNS) {
				cga_cursor_x = 0;
				cga_newline();
			}
	}
	cga_move_cursor(cga_cursor_x, cga_cursor_y);
}


/*
	@brief CGA 80*25 打印字符串。
*/
void cga_puts(const char *str)
{
	while (*str)
		cga_putchar(*str++);
}

/*
	@brief CGA 80*25 打印字符串。
	@note 最大长度为 max_length = 1024。
*/
int cga_printf(const char *format, ...)
{
	const size_t max_length = 1024;
	char buf[max_length];

	int result;
	va_list va;
	va_start(va, format);

	result = vsnprintf(buf, max_length, format, va);
	va_end(va);

	cga_puts(buf);
	return result;
}
