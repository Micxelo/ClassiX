/*
	include/ClassiX/mouse.h
*/

#ifndef _CLASSIX_MOUSE_H_
#define _CLASSIX_MOUSE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>
#include <ClassiX/window.h>

#define MOUSE_LBUTTON						(0b00000001)
#define MOUSE_RBUTTON						(0b00000010)
#define MOUSE_MBUTTON						(0b00000100)

typedef struct {
	uint8_t phase;
	int8_t dx, dy, dz;
	uint8_t button;
} MOUSE_DATA;

void init_mouse(FIFO *fifo, int32_t data0);
int32_t mouse_decoder(MOUSE_DATA *mouse_data, uint8_t data);

#define MOUSE_DATA0							(512)		/* 鼠标数据起始索引 */

/* 鼠标按钮状态 */
typedef struct {
	/* 双击判定用的历史数据 */
	uint32_t last_down_time;	/* 上一次按下的时间 */
	int32_t last_down_x;		/* 上一次按下的 X 坐标 (绝对坐标) */
	int32_t last_down_y;		/* 上一次按下的 Y 坐标 (绝对坐标) */

	/* 挂起的单击事件数据 */
	bool pending;				/* 是否有一个等待确认的单击 */
	uint32_t pending_time;		/* 松开的时间点 */
	int32_t pending_x;			/* 松开时的相对 X 坐标 */
	int32_t pending_y;			/* 松开时的相对 Y 坐标 */
	WINDOW *target_win;			/* 目标窗口 */

	bool double_clicked;		/* 本次按下是否已触发双击 */
} BTN_CLICK_STATE;

/* 鼠标状态管理器 */
typedef struct {
	int32_t last_buttons;	/* 上一次的按键掩码 */

	BTN_CLICK_STATE left;	/* 左键点击状态 */
	BTN_CLICK_STATE right;	/* 右键点击状态 */
	BTN_CLICK_STATE middle;	/* 中键点击状态 */
} MOUSE_STATE_MANAGER;

#ifdef __cplusplus
	}
#endif

#endif
