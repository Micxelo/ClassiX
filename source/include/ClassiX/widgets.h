/*
	include/ClassiX/widgets.h
*/

#ifndef _CLASSIX_WIDGETS_H_
#define _CLASSIX_WIDGETS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/fifo.h>
#include <ClassiX/layer.h>
#include <ClassiX/typedef.h>

typedef enum {
	CURSOR_ARROW = 0,
	CURSOR_BEAM,
	CURSOR_BUSY,
	CURSOR_LINK
} CURSOR;

LAYER *cursor_init(void);
void cursor_set(CURSOR type);

typedef enum {
	WIDGET_TYPE_WIDGET  = 0x85f91ed0,		/* 基类 */
	WIDGET_TYPE_WINDOW  = 0x8be4f9dd,		/* 窗口 */
	WIDGET_TYPE_LABEL   = 0x0ea750e8,		/* 标签 */
	WIDGET_TYPE_BUTTON  = 0x3a06ac3d,		/* 按钮 */
	WIDGET_TYPE_TEXTBOX = 0x0765b842		/* 文本框 */
} WIDGET_TYPE;

typedef struct {
	uint32_t id;
	void *param;
} EVENT;

typedef struct WIDGET {
	WIDGET_TYPE type;
	uint16_t width;
	uint16_t height;
	int16_t x;
	int16_t y;
	void *data;
	struct WIDGET *parent;
	struct WIDGET *first_child;
	struct WIDGET *next_sibling;
	void (*create)(struct WIDGET *self);
	void (*process)(struct WIDGET *self, EVENT *event);
	void (*destroy)(struct WIDGET *self);
} WIDGET;

/* 部件类型转换宏 */
#define WIDGET_CAST(TYPE, ptr) \
	(((ptr) && (ptr)->type == WIDGET_TYPE_##TYPE) ? (TYPE*) (ptr) : NULL)

typedef enum {
	WINSTYLE_SYSCONTROL = 0x00000001,		/* 窗口有系统控件 */
	WINSTYLE_MINIMIZABLE = 0x00000002,		/* 窗口有最小化按钮 */
	WINSTYLE_ICON = 0x00000004,				/* 窗口有图标 */
	WINSTYLE_MOVEABLE = 0x00000008,			/* 窗口可移动 */
	WINSTYLE_TOPMOST = 0x00000010,			/* 窗口总在最上层 */
	WINSTYLE_SHOWINTASKBAR = 0x0000020,		/* 窗口在任务栏显示 */
	WINSTYLE_MINIMIZED = 0x00000040,		/* 窗口已最小化 */
} WINSTYLE;

typedef struct {
	WIDGET base;
	LAYER *layer;
	FIFO event_queue;
} WINDOW;

#ifdef __cplusplus
	}
#endif

#endif