/*
	include/ClassiX/events.h
*/

#ifndef _CLASSIX_EVENTS_H_
#define _CLASSIX_EVENTS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "typedef.h"

typedef enum {
	EVENT_NULL = 0,				/* 无效事件 | 占位 */

	/* 窗口管理事件 */
	EVENT_WINDOW_CREATED,		/* 窗口已创建 | 窗口句柄 */
	EVENT_WINDOW_PAINT,			/* 窗口绘制 | 窗口句柄 */
	EVENT_WINDOW_CLOSING,		/* 窗口正在关闭 | 关闭原因 */
	EVENT_WINDOW_DESTROYED,		/* 窗口已销毁 | 占位 */
	EVENT_WINDOW_GOTFOCUS,		/* 窗口获得焦点 | 占位 */
	EVENT_WINDOW_LOSTFOCUS,		/* 窗口失去焦点 | 占位 */
	EVENT_WINDOW_MOVING,		/* 窗口正在移动 | 此时坐标 */

	/* 键盘事件 */
	EVENT_KEYBOARD_KEYDOWN,		/* 按键按下 | 虚拟键码 */
	EVENT_KEYBOARD_KEYUP,		/* 按键抬起 | 占位 */
	EVENT_KEYBOARD_KEYPRESS,	/* 按键输入 | 字符代码 */

	/* 鼠标事件 */
	EVENT_MOUSE_MOVE,			/* 光标移动 | 光标相对坐标 */
	EVENT_MOUSE_LBTNDOWN,		/* 鼠标左键按下 | 光标相对坐标 */
	EVENT_MOUSE_LBTNUP,			/* 鼠标左键抬起 | 光标相对坐标 */
	EVENT_MOUSE_RBTNDOWN,		/* 鼠标右键按下 | 光标相对坐标 */
	EVENT_MOUSE_RBTNUP,			/* 鼠标右键抬起 | 光标相对坐标 */
	EVENT_MOUSE_MBTNDOWN,		/* 鼠标中键按下 | 光标相对坐标 */
	EVENT_MOUSE_MBTNUP,			/* 鼠标中键抬起 | 光标相对坐标 */
	EVENT_MOUSE_LCLICK,			/* 鼠标左键单击 | 光标相对坐标 */
	EVENT_MOUSE_RCLICK,			/* 鼠标右键单击 | 光标相对坐标 */
	EVENT_MOUSE_MCLICK,			/* 鼠标中键单击 | 光标相对坐标 */
	EVENT_MOUSE_LDBCLK,			/* 鼠标左键双击 | 光标相对坐标 */
	EVENT_MOUSE_RDBCLK,			/* 鼠标右键双击 | 光标相对坐标 */
	EVENT_MOUSE_MDBCLK,			/* 鼠标中键双击 | 光标相对坐标 */
	EVENT_MOUSE_WHEEL			/* 鼠标滚轮 | 滚轮增量 */
} EVENT_ID;

extern void cx_wait_event(uint32_t *id, uint32_t *param);

#ifdef __cplusplus
	}
#endif

#endif
