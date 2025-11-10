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
	WIDGET_TYPE_WIDGET  = 0x85f91ed0,
	WIDGET_TYPE_WINDOW  = 0x8be4f9dd,
	WIDGET_TYPE_LABEL   = 0x0ea750e8,
	WIDGET_TYPE_BUTTON  = 0x3a06ac3d,
	WIDGET_TYPE_TEXTBOX = 0x0765b842
} WIDGET_TYPE;

typedef struct {
	uint32_t id;
	void *param;
} EVENT;

typedef struct WIDGET {
	WIDGET_TYPE type;
	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	char *text;
	struct WIDGET *parent;
	struct WIDGET *first_child;
	struct WIDGET *next_sibling;
	void (*create)(struct WIDGET *self);
	void (*process)(struct WIDGET *self, EVENT *event);
	void (*destroy)(struct WIDGET *self);
} WIDGET;

typedef enum {
	WINSTYLE_SYSCONTROL = 0x00000001,		/* 窗口有系统控件 */
	WINSTYLE_MINIMIZABLE = 0x00000002,		/* 窗口有最小化按钮 */
	WINSTYLE_MAXIMIZABLE = 0x00000004,		/* 窗口有最大化按钮 */
	WINSTYLE_ICON = 0x00000008,				/* 窗口有图标 */
	WINSTYLE_MOVEABLE = 0x00000010,			/* 窗口可移动 */
	WINSTYLE_RESIZABLE = 0x00000020,		/* 窗口可调整大小 */
	WINSTYLE_TOPMOST = 0x00000040,			/* 窗口总在最上层 */
	WINSTYLE_SHOWINTASKBAR = 0x00000080,	/* 窗口在任务栏显示 */
	WINSTYLE_MINIMIZED = 0x00000100,		/* 窗口已最小化 */
	WINSTYLE_MAXIMIZED = 0x00000200,		/* 窗口已最大化 */
	WINSTYLE_FULLSCREEN = 0x00000400,		/* 窗口已全屏 */
} WINSTYLE;

typedef struct {
	WIDGET base;
	LAYER *layer;
	FIFO event_queue;
} WINDOW;

#define WIDGET_CAST(TYPE, ptr) \
	((ptr) && (ptr)->type == WIDGET_TYPE_##TYPE) ? (TYPE*) (ptr) : NULL

#ifdef __cplusplus
	}
#endif

#endif