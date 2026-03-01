/*
	include/ClassiX/window.h
*/

#ifndef _CLASSIX_WINDOW_H_
#define _CLASSIX_WINDOW_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/font.h>
#include <ClassiX/layer.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

typedef struct TASK TASK;

/* 光标类型 */
typedef enum {
	CURSOR_ARROW = 0,
	CURSOR_BEAM,
	CURSOR_BUSY
} CURSOR;

/* CSR 文件头 */
typedef struct __attribute__((packed)) {
	uint32_t magic;			/* 魔数 */
	uint16_t width;			/* 光标宽度 */
	uint16_t height;		/* 光标高度 */
	uint16_t anchor_x;		/* 判定点 X */
	uint16_t anchor_y;		/* 判定点 Y */
	uint32_t reserved;
} CSR_HEADER;

extern CSR_HEADER *cursor_data[];
extern CSR_HEADER *cursor_current;

LAYER *cursor_init(void);
void cursor_set(CURSOR type);

#define WINDOW_STRUCT_SIGNATURE				(0x85f91ed0)

enum {
	WD_SUCCESS = 0,					/* 成功 */
	WD_INVALID_PARAM,				/* 参数无效 */
	WD_NO_MEMORY					/* 内存不足 */
};

/* 窗口样式 */
typedef enum {
	WINSTYLE_SYSCONTROL = 0x00000001,		/* 窗口有系统控件 */
	WINSTYLE_MINIMIZABLE = 0x00000002,		/* 窗口有最小化按钮 */
	WINSTYLE_ICON = 0x00000004,				/* 窗口有图标 */
	WINSTYLE_MOVEABLE = 0x00000008,			/* 窗口可移动 */
	WINSTYLE_TOPMOST = 0x00000010,			/* 窗口总在最上层 */
	WINSTYLE_SHOWINTASKBAR = 0x00000020,	/* 窗口在任务栏显示 */
	WINSTYLE_MINIMIZED = 0x00000040,		/* 窗口已最小化 */

	/* 私有属性，仅供系统使用 */
	WINSTYLE_BOTTOMMOST = 0x40000000		/* 窗口总在最下层 */
} WINSTYLE;

typedef enum {
	STARTUP_POS_CASCADE = 0,		/* 级联位置 */
	STARTUP_POS_CENTER,				/* 居中位置 */
	STARTUP_POS_MANUAL				/* 手动指定位置 */
} STARTUP_POS;

typedef struct WINDOW {
	uint32_t signature;		/* 窗口签名 */
	uint16_t width;			/* 窗口宽度 */
	uint16_t height;		/* 窗口高度 */
	int16_t x;				/* 窗口 X 坐标 */
	int16_t y;				/* 窗口 Y 坐标 */
	WINSTYLE style;			/* 窗口样式 */
	int16_t client_x;		/* 客户区相对于窗口的 X 坐标 */
	int16_t client_y;		/* 客户区相对于窗口的 Y 坐标 */
	int16_t client_width;	/* 客户区宽度 */
	int16_t client_height;	/* 客户区高度 */
	const char *title;		/* 标题 */
	LAYER *layer;			/* 窗口图层 */
	TASK *task;				/* 关联的任务 */
} WINDOW;

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
} EVENT_TYPE;

#define CLOSING_BY_CLOSE_BUTTON		1	/* 通过点击关闭按钮关闭 */
#define CLOSING_BY_TASK_REQUEST		2	/* 通过关联的任务请求关闭 */
#define CLOSING_BY_TASK_EXIT		3	/* 通过关联的任务退出关闭 */

/* 事件结构体 */
typedef struct {
	WINDOW *window;
	uint32_t id;
	union {
		uint32_t param;
		struct __attribute__((aligned(16))) { uint16_t a, b; } tuple;
		struct __attribute__((aligned(16))) { int16_t x, y; } point;
	};
} EVENT;

/* 命中测试结果 */
typedef enum {
	HIT_NONE = 0,			/* 不在窗口内 */
	HIT_CLIENT,				/* 客户区 */
	HIT_TITLEBAR,			/* 标题栏（非按钮区域） */
	HIT_CLOSE_BUTTON,		/* 关闭按钮 */
	HIT_MINIMIZE_BUTTON,	/* 最小化按钮 */
	HIT_ICON,				/* 窗口图标 */

	HIT_BORDER_LEFT,		/* 左边框 */
	HIT_BORDER_RIGHT,		/* 右边框 */
	HIT_BORDER_TOP,			/* 上边框 */
	HIT_BORDER_BOTTOM,		/* 下边框 */
	HIT_BORDER_TOPLEFT,		/* 左上角顶点 */
	HIT_BORDER_TOPRIGHT,	/* 右上角顶点 */
	HIT_BORDER_BOTTOMLEFT,	/* 左下角顶点 */
	HIT_BORDER_BOTTOMRIGHT,	/* 右下角顶点 */
} HIT_RESULT;

int32_t window_create(WINDOW *window, int16_t x, int16_t y, uint16_t width, uint16_t height, uint32_t style, STARTUP_POS startup, const char *title, TASK *task);
void window_paint(WINDOW *window);
void window_destroy(WINDOW *window);
void window_focus(LAYER **focused_layer, LAYER *new_layer);
void window_activate(WINDOW *window);
void window_inactivate(WINDOW *window);
void window_set_position(WINDOW *window, int16_t x, int16_t y);
void window_get_cascade_position(int16_t *x, int16_t *y, uint16_t width, uint16_t height);
HIT_RESULT window_hit_test(WINDOW *window, int16_t x, int16_t y);

#define window_draw_rectangle(_window, _x, _y, _width, _height, _thickness, _color) \
	draw_rectangle((_window)->layer->buf, (_window)->width, (_x), (_y), (_width), (_height), (_thickness), (_color))
#define window_draw_rectangle_by_corners(_window, _x0, _y0, _x1, _y1, _thickness, _color) \
	draw_rectangle_by_corners((_window)->layer->buf, (_window)->width, (_x0), (_y0), (_x1), (_y1), (_thickness), (_color))
#define window_fill_rectangle(_window, _x, _y, _width, _height, _color) \
	fill_rectangle((_window)->layer->buf, (_window)->width, (_x), (_y), (_width), (_height), (_color))
#define window_fill_rectangle_by_corners(_window, _x0, _y0, _x1, _y1, _color) \
	fill_rectangle_by_corners((_window)->layer->buf, (_window)->width, (_x0), (_y0), (_x1), (_y1), (_color))
#define window_draw_line(_window, _x0, _y0, _x1, _y1, _color) \
	draw_line((_window)->layer->buf, (_window)->width, (_x0), (_y0), (_x1), (_y1), (_color))
#define window_draw_circle(_window, _x0, _y0, _radius, _color) \
	draw_circle((_window)->layer->buf, (_window)->width, (_x0), (_y0), (_radius), (_color))
#define window_fill_circle(_window, _x0, _y0, _radius, _color) \
	fill_circle((_window)->layer->buf, (_window)->width, (_x0), (_y0), (_radius), (_color))
#define window_draw_ellipse(_window, _x0, _y0, _a, _b, _color) \
	draw_ellipse((_window)->layer->buf, (_window)->width, (_x0), (_y0), (_a), (_b), (_color))
#define window_fill_ellipse(_window, _x0, _y0, _a, _b, _color) \
	fill_ellipse((_window)->layer->buf, (_window)->width, (_x0), (_y0), (_a), (_b), (_color))
#define window_draw_ascii_string(_window, _x, _y, _color, _str, _font) \
	draw_ascii_string((_window)->layer->buf, (_window)->width, (_x), (_y), (_color), (_str), (_font))
#define window_draw_unicode_string(_window, _x, _y, _color, _str, _font) \
	draw_unicode_string((_window)->layer->buf, (_window)->width, (_x), (_y), (_color), (_str), (_font))

#define client_draw_rectangle(_window, _x, _y, _width, _height, _thickness, _color) \
	draw_rectangle((_window)->layer->buf, (_window)->width, (_x) + (_window)->client_x, (_y) + (_window)->client_y, (_width), (_height), (_thickness), (_color))
#define client_draw_rectangle_by_corners(_window, _x0, _y0, _x1, _y1, _thickness, _color) \
	draw_rectangle_by_corners((_window)->layer->buf, (_window)->width, (_x0) + (_window)->client_x, (_y0) + (_window)->client_y, (_x1) + (_window)->client_x, (_y1) + (_window)->client_y, (_thickness), (_color))
#define client_fill_rectangle(_window, _x, _y, _width, _height, _color) \
	fill_rectangle((_window)->layer->buf, (_window)->width, (_x) + (_window)->client_x, (_y) + (_window)->client_y, (_width), (_height), (_color))
#define client_fill_rectangle_by_corners(_window, _x0, _y0, _x1, _y1, _color) \
	fill_rectangle_by_corners((_window)->layer->buf, (_window)->width, (_x0) + (_window)->client_x, (_y0) + (_window)->client_y, (_x1) + (_window)->client_x, (_y1) + (_window)->client_y, (_color))
#define client_draw_line(_window, _x0, _y0, _x1, _y1, _color) \
	draw_line((_window)->layer->buf, (_window)->width, (_x0) + (_window)->client_x, (_y0) + (_window)->client_y, (_x1) + (_window)->client_x, (_y1) + (_window)->client_y, (_color))
#define client_draw_circle(_window, _x0, _y0, _radius, _color) \
	draw_circle((_window)->layer->buf, (_window)->width, (_x0) + (_window)->client_x, (_y0) + (_window)->client_y, (_radius), (_color))
#define client_fill_circle(_window, _x0, _y0, _radius, _color) \
	fill_circle((_window)->layer->buf, (_window)->width, (_x0) + (_window)->client_x, (_y0) + (_window)->client_y, (_radius), (_color))
#define client_draw_ellipse(_window, _x0, _y0, _a, _b, _color) \
	draw_ellipse((_window)->layer->buf, (_window)->width, (_x0) + (_window)->client_x, (_y0) + (_window)->client_y, (_a), (_b), (_color))
#define client_fill_ellipse(_window, _x0, _y0, _a, _b, _color) \
	fill_ellipse((_window)->layer->buf, (_window)->width, (_x0) + (_window)->client_x, (_y0) + (_window)->client_y, (_a), (_b), (_color))
#define client_draw_ascii_string(_window, _x, _y, _color, _str, _font) \
	draw_ascii_string((_window)->layer->buf, (_window)->width, (_x) + (_window)->client_x, (_y) + (_window)->client_y, (_color), (_str), (_font))
#define client_draw_unicode_string(_window, _x, _y, _color, _str, _font) \
	draw_unicode_string((_window)->layer->buf, (_window)->width, (_x) + (_window)->client_x, (_y) + (_window)->client_y, (_color), (_str), (_font))

#ifdef __cplusplus
	}
#endif

#endif