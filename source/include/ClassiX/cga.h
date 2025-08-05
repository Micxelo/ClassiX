/*
	include/ClassiX/cga.h
*/

#ifndef _CLASSIX_CGA_H_
#define _CLASSIX_CGA_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define CGA_COLUMNS						80
#define CGA_ROWS						25
#define CGA_BUF_ADDR					0x000b8000

#define CGA_BLACK						0	/* CGA 黑 */
#define CGA_BLUE						1	/* CGA 蓝 */
#define CGA_GREEN						2	/* CGA 绿 */
#define CGA_CYAN						3	/* CGA 青 */
#define CGA_RED							4	/* CGA 红 */
#define CGA_MAGENTA						5	/* CGA 品红 */
#define CGA_BROWN						6	/* CGA 棕 */
#define CGA_LIGHTGRAY					7	/* CGA 浅灰 */
#define CGA_DARKGRAY					8	/* CGA 暗灰 */
#define CGA_LIGHTBLUE					9	/* CGA 浅蓝 */
#define CGA_LIGHTGREEN					10	/* CGA 浅绿 */
#define CGA_LIGHTCYAN					11	/* CGA 浅青 */
#define CGA_LIGHTRED					12	/* CGA 浅红 */
#define CGA_LIGHTMAGENTA				13	/* CGA 浅品红 */
#define CGA_YELLOW						14	/* CGA 黄 */
#define CGA_WHITE						15	/* CGA 白 */

void set_cga_cursor_pos(uint16_t x, uint16_t y);
uint16_t get_cga_cursor_pos(void);
void set_cga_color(uint16_t color);
void cga_clear(void);
void cga_move_cursor(uint16_t x, uint16_t y);
void cga_hide_cursor(void);
void cga_show_cursor(void);
void cga_putchar(char c);
void cga_puts(const char *str);
int cga_printf(const char *format, ...)  __attribute__((format(printf, 1, 2)));

#ifdef __cplusplus
	}
#endif

#endif
