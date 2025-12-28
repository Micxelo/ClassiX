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
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

/* 光标类型 */
typedef enum {
	CURSOR_ARROW = 0,
	CURSOR_BEAM,
	CURSOR_BUSY,
	CURSOR_LINK
} CURSOR;

LAYER *cursor_init(void);
void cursor_set(CURSOR type);

/* 部件类型 */
typedef enum {
	WIDGET_TYPE_WIDGET  = 0x85f91ed0,		/* 基类 */
	WIDGET_TYPE_WINDOW  = 0x8be4f9dd,		/* 窗口 */
	WIDGET_TYPE_LABEL   = 0x0ea750e8,		/* 标签 */
	WIDGET_TYPE_BUTTON  = 0x3a06ac3d,		/* 按钮 */
	WIDGET_TYPE_TEXTBOX = 0x0765b842		/* 文本框 */
} WIDGET_TYPE;

/* 事件结构体 */
typedef struct {
	uint32_t id;
	void *param;
} EVENT;

typedef struct WIDGET {
	WIDGET_TYPE type;				/* 部件类型 */
	uint16_t width;					/* 部件宽度 */
	uint16_t height;				/* 部件高度 */
	int16_t x;						/* 相对于父部件的 X 坐标 */
	int16_t y;						/* 相对于父部件的 Y 坐标 */
	uint32_t style;					/* 部件样式 */
	void *data;						/* 私有数据 */
	struct WIDGET *parent;			/* 父部件 */
	struct WIDGET *first_child;		/* 第一个子部件 */
	struct WIDGET *next_sibling;	/* 下一个同级部件 */
	void (*process)(struct WIDGET *self, EVENT *event);
} WIDGET;

#define WIDGET_CAST(TYPE, ptr) \
	(((ptr) && (ptr)->type == WIDGET_TYPE_##TYPE) ? (TYPE*) (ptr) : NULL)

/* WIDGET 错误码 */
enum {
	WD_SUCCESS = 0,					/* 成功 */
	WD_INVALID_PARAM,				/* 参数无效 */
	WD_NO_MEMORY,					/* 内存不足 */
};

void wd_draw_convex_string(LAYER *layer, int32_t x, int32_t y, COLOR fg_color, COLOR bg_color, const char *str, const BITMAP_FONT *font);

/* 窗口样式 */
typedef enum {
	WINSTYLE_SYSCONTROL = 0x00000001,		/* 窗口有系统控件 */
	WINSTYLE_MINIMIZABLE = 0x00000002,		/* 窗口有最小化按钮 */
	WINSTYLE_ICON = 0x00000004,				/* 窗口有图标 */
	WINSTYLE_MOVEABLE = 0x00000008,			/* 窗口可移动 */
	WINSTYLE_TOPMOST = 0x00000010,			/* 窗口总在最上层 */
	WINSTYLE_SHOWINTASKBAR = 0x00000020,	/* 窗口在任务栏显示 */
	WINSTYLE_MINIMIZED = 0x00000040,		/* 窗口已最小化 */
} WINSTYLE;

typedef struct WINDOW {
	WIDGET base;		/* 基类 */
	LAYER *layer;		/* 窗口图层 */
} WINDOW;

#define WD_WINDOW_IS_CLICKING_CLOSE_BUTTON(win, x, y)							\
	((win)->base.style & WINSTYLE_SYSCONTROL &&									\
	 (x) >= (win)->base.width - 24 - 2 && (x) <= (win)->base.width - 2 &&		\
	 (y) >= 3 && (y) <= 18)

#define WD_WINDOW_IS_CLICKING_MINIMIZE_BUTTON(win, x, y)						\
	((win)->base.style & WINSTYLE_SYSCONTROL &&									\
	 (win)->base.style & WINSTYLE_MINIMIZABLE &&								\
	 (x) >= (win)->base.width - 48 - 2 && (x) <= (win)->base.width - 24 - 2 &&	\
	 (y) >= 3 && (y) <= 18)

#define WD_WINDOW_IS_HOLDING_TITLEBAR(win, x, y)					\
	((win)->base.style & WINSTYLE_MOVEABLE &&						\
	 (!(WD_WINDOW_IS_CLICKING_CLOSE_BUTTON((win), (x), (y)))) &&	\
	 (!(WD_WINDOW_IS_CLICKING_MINIMIZE_BUTTON((win), (x), (y)))) &&	\
	 (y) >= 1 && (y) <= 18 &&										\
	 (x) >= 1 && (x) <= (win)->base.width - 2)

int32_t window_create(WINDOW *window, int16_t x, int16_t y, uint16_t width, uint16_t height, uint32_t style, const char *title, TASK *task);
int32_t window_destroy(WINDOW *window);

#ifdef __cplusplus
	}
#endif

#endif