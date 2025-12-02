/*
	include/ClassiX/keyboard.h
*/

#ifndef _CLASSIX_KEYBOARD_H_
#define _CLASSIX_KEYBOARD_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>

#define EXPANDED_KEY_PREFIX					(0xe0)			/* 扩展键前缀 */
#define KEYCMD_LED							(0xed)			/* 设置 LED 指令 */
#define KEYBOARD_LED_SCROLL_LOCK 			(0b00000001)	/* 滚动锁定 */
#define KEYBOARD_LED_NUM_LOCK 				(0b00000010)	/* 数字锁定 */
#define KEYBOARD_LED_CAPS_LOCK 				(0b00000100)	/* 大写锁定 */

extern const uint8_t keymap_us_default[];
extern const uint8_t keymap_us_shift[];

void init_keyboard(FIFO *fifo, int32_t data0);
void kbc_wait_ready(void);
void kbc_send_data(uint8_t data);

#ifdef __cplusplus
	}
#endif

#endif
