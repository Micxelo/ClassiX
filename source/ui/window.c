/*
	ui/window.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/debug.h>
#include <ClassiX/fifo.h>
#include <ClassiX/font.h>
#include <ClassiX/framebuf.h>
#include <ClassiX/graphic.h>
#include <ClassiX/layer.h>
#include <ClassiX/memory.h>
#include <ClassiX/palette.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>
#include <ClassiX/window.h>

#include <string.h>

#define CSR_MAGIC							(0xc2d36fb8)

CSR_HEADER *cursor_data[] = {
	(CSR_HEADER *) ASSET_DATA(cursors, arrow_csr),
	(CSR_HEADER *) ASSET_DATA(cursors, beam_csr),
	(CSR_HEADER *) ASSET_DATA(cursors, busy_csr)
};
CSR_HEADER *cursor_current;

static LAYER *cursor_layer;

/*
	@brief 初始化鼠标光标。
	@return 光标图层指针，失败返回 NULL
*/
LAYER *cursor_init(void)
{
	cursor_current = cursor_data[CURSOR_ARROW];
	if (cursor_current->magic != CSR_MAGIC)
		return NULL;

	cursor_layer = layer_alloc(cursor_current->width, cursor_current->height, true);
	if (!cursor_layer)
		return NULL;

	memcpy(cursor_layer->buf, (uint8_t *) (cursor_current + 1), cursor_current->width * cursor_current->height * sizeof(COLOR));

	return cursor_layer;
}

/*
	@brief 设置鼠标光标形状。
	@param type 光标类型
*/
void cursor_set(CURSOR type)
{
	CSR_HEADER *csr;

	if (type >= sizeof(cursor_data) / sizeof(cursor_data[0]))
		return;

	csr = (CSR_HEADER *) cursor_data[type];
	if (csr->magic != CSR_MAGIC)
		return;

	cursor_current = csr;

	memcpy(cursor_layer->buf, (uint8_t *) (cursor_current + 1), cursor_current->width * cursor_current->height * sizeof(COLOR));
}

#define DEFAULT_FONT_WIDTH					(8)		/* 默认字体宽度 */
#define DEFAULT_FONT_HEIGHT					(16)	/* 默认字体高度 */

#define WINDOW_BORDER_WIDTH					(1)		/* 窗口边框宽度 */
#define WINDOW_TITLEBAR_HEIGHT				(18)	/* 窗口标题栏高度 */
#define WINDOW_TITLETEXT_YPOS				((WINDOW_TITLEBAR_HEIGHT - DEFAULT_FONT_HEIGHT) / 2 + WINDOW_BORDER_WIDTH)
#define WINDOW_ICON_WIDTH					(16)	/* 窗口图标宽度 */

LAYER *layer_focused = NULL;	/* 获得焦点的窗口图层 */

/*
	@brief 绘制带阴影效果的字符串。
	@param layer 指向图层结构的指针
	@param x 字符串的 X 坐标
	@param y 字符串的 Y 坐标
	@param fg_color 前景色
	@param bg_color 阴影色
	@param str 要绘制的字符串
	@param font 字体信息
*/
static void draw_convex_string(WINDOW *window, int32_t x, int32_t y, COLOR fg_color, COLOR bg_color, const char *str, const BITMAP_FONT *font)
{
	/* 绘制阴影 */
	window_draw_unicode_string(window, x + 1, y + 1, bg_color, str, font);

	/* 绘制前景文字 */
	window_draw_unicode_string(window, x, y, fg_color, str, font);
}

/*
	@brief 绘制窗口标题栏。
	@param window 指向窗口结构的指针
	@param active 是否为活动窗口
*/
static void window_draw_titlebar(WINDOW *window, bool active)
{
	/* 绘制标题栏 */
	window_fill_rectangle(window,
		WINDOW_BORDER_WIDTH,
		WINDOW_BORDER_WIDTH,
		window->width - 2 * WINDOW_BORDER_WIDTH,
		WINDOW_TITLEBAR_HEIGHT,
		active ? COLOR_SYSTEM_WINDOW_TITLEBAR_ACTIVE : COLOR_SYSTEM_WINDOW_TITLEBAR_INACTIVE);

	/* 绘制关闭按钮 */
	draw_convex_string(window,
		window->width - 3 * DEFAULT_FONT_WIDTH - 2 * WINDOW_BORDER_WIDTH,
		WINDOW_TITLETEXT_YPOS,
		active ? COLOR_SYSTEM_WINDOW_TEXT_ACTIVE : COLOR_SYSTEM_WINDOW_TEXT_INACTIVE,
		active ? COLOR_SYSTEM_WINDOW_TEXT_INACTIVE : COLOR_SYSTEM_WINDOW_TEXT_ACTIVE,
		"[X]",
		&font_terminus_16b);

	if (window->style & WINSTYLE_MINIMIZABLE)
		/* 绘制最小化按钮 */
		draw_convex_string(window,
			window->width - 6 * DEFAULT_FONT_WIDTH - 2 * WINDOW_BORDER_WIDTH,
			WINDOW_TITLETEXT_YPOS,
			active ? COLOR_SYSTEM_WINDOW_TEXT_ACTIVE : COLOR_SYSTEM_WINDOW_TEXT_INACTIVE,
			active ? COLOR_SYSTEM_WINDOW_TEXT_INACTIVE : COLOR_SYSTEM_WINDOW_TEXT_ACTIVE,
			"[-]",
			&font_terminus_16b);

	if (window->title)
		/* 绘制标题文字 */
		draw_convex_string(window,
			((window->style & WINSTYLE_ICON) ? WINDOW_BORDER_WIDTH + 1 + WINDOW_ICON_WIDTH : WINDOW_BORDER_WIDTH + 1),
			WINDOW_TITLETEXT_YPOS,
			active ? COLOR_SYSTEM_WINDOW_TEXT_ACTIVE : COLOR_SYSTEM_WINDOW_TEXT_INACTIVE,
			active ? COLOR_SYSTEM_WINDOW_TEXT_INACTIVE : COLOR_SYSTEM_WINDOW_TEXT_ACTIVE,
			window->title,
			&font_terminus_16b);
}

/*
	@brief 创建窗口。
	@param window 指向窗口结构的指针
	@param x 窗口的初始 X 坐标
	@param y 窗口的初始 Y 坐标
	@param width 客户区宽度
	@param height 客户区高度
	@param style 窗口样式
	@param title 窗口标题
	@param task 关联的任务
	@return 错误码
*/
int32_t window_create(WINDOW *window, int16_t x, int16_t y, uint16_t width, uint16_t height, uint32_t style, STARTUP_POS startup, const char *title, TASK *task)
{
	if (!window)
		return WD_INVALID_PARAM; /* 参数无效 */

	window->signature = WINDOW_STRUCT_SIGNATURE;
	window->x = x;
	window->y = y;
	window->width = width + ((style & WINSTYLE_SYSCONTROL) ? 2 * WINDOW_BORDER_WIDTH : 0);
	window->height = height + ((style & WINSTYLE_SYSCONTROL) ? 2 * WINDOW_BORDER_WIDTH + WINDOW_TITLEBAR_HEIGHT : 0);
	window->style = style;
	window->title = title;

	/* 创建图层 */
	window->layer = layer_alloc(window->width, window->height, false);
	if (!window->layer)
		return WD_NO_MEMORY; /* 内存不足 */

	window->layer->window = window;

	/* 确定窗口位置 */
	int16_t pos_x, pos_y;
	switch (startup) {
		case STARTUP_POS_CASCADE:
			window_get_cascade_position(&pos_x, &pos_y, window->width, window->height);
			break;
		case STARTUP_POS_CENTER:
			pos_x = (g_fb.width - window->width) / 2;
			pos_y = (g_fb.height - window->height) / 2;
			break;
		case STARTUP_POS_MANUAL:
		default:
			pos_x = x;
			pos_y = y;
			break;
	}
	window_set_position(window, pos_x, pos_y);

	window->task = task;
	window->client_x = (style & WINSTYLE_SYSCONTROL) ? WINDOW_BORDER_WIDTH : 0;
	window->client_y = (style & WINSTYLE_SYSCONTROL) ? WINDOW_BORDER_WIDTH + WINDOW_TITLEBAR_HEIGHT : 0;
	window->client_width = width;
	window->client_height = height;

	/* 发送窗口创建事件 */
	if (task) {
		EVENT event = { .window = window, .id = EVENT_WINDOW_CREATED };
		fifo_push_event(&task->fifo, &event);
	}

	/* 绘制窗口 */
	window_paint(window);
	layer_refresh(window->layer, 0, 0, window->width - 1, window->height - 1);

	/* 发送窗口绘制事件 */
	if (task) {
		EVENT event = { .window = window, .id = EVENT_WINDOW_PAINT };
		fifo_push_event(&task->fifo, &event);
	}

	return WD_SUCCESS;
}

/*
	@brief 绘制窗口。
	@param window 指向窗口结构的指针
*/
void window_paint(WINDOW *window)
{
	/* 绘制窗口背景 */
	window_fill_rectangle(window, 0, 0, window->width, window->height, COLOR_SYSTEM_WINDOW_BACKGROUND);

	if (window->style & WINSTYLE_SYSCONTROL) {
		/* 绘制边框 */
		window_draw_rectangle(window,
			0,
			0,
			window->width,
			window->height,
			WINDOW_BORDER_WIDTH,
			COLOR_SYSTEM_WINDOW_FRAME);

		/* 绘制标题栏 */
		window_draw_titlebar(window, false);
	}
}

/*
	@brief 销毁窗口。
	@param window 指向窗口结构的指针
*/
void window_destroy(WINDOW *window)
{
	/* 释放图层 */
	layer_free(window->layer);
	window->layer = NULL;

	return;
}

/*
	@brief 设置窗口焦点。
	@param focused_layer 指向当前获得焦点的窗口图层指针
	@param new_layer 新的获得焦点的窗口图层
*/
void window_focus(LAYER **focused_layer, LAYER *new_layer)
{
	if (*focused_layer == new_layer)
		return; /* 已经是焦点 */

	if (*focused_layer) {
		/* 失去焦点，重绘标题栏 */
		WINDOW *old_window = (*focused_layer)->window;
		if (old_window && old_window->style & WINSTYLE_SYSCONTROL) {
			window_draw_titlebar(old_window, false);
			layer_refresh((*focused_layer),
				WINDOW_BORDER_WIDTH,
				WINDOW_BORDER_WIDTH,
				(*focused_layer)->width - WINDOW_BORDER_WIDTH - 1,
				WINDOW_TITLEBAR_HEIGHT + WINDOW_BORDER_WIDTH - 1);
		}

		/* 发送窗口失去焦点事件 */
		if (old_window && old_window->task) {
			EVENT event = { .window = old_window, .id = EVENT_WINDOW_LOSTFOCUS };
			fifo_push_event(&old_window->task->fifo, &event);
		}
	}

	*focused_layer = new_layer;

	if (*focused_layer) {
		/* 获得焦点，重绘标题栏 */
		WINDOW *new_window = (*focused_layer)->window;
		if (new_window && new_window->style & WINSTYLE_SYSCONTROL) {
			window_draw_titlebar(new_window, true);
			layer_refresh((*focused_layer),
				WINDOW_BORDER_WIDTH,
				WINDOW_BORDER_WIDTH,
				(*focused_layer)->width - WINDOW_BORDER_WIDTH - 1,
				WINDOW_TITLEBAR_HEIGHT + WINDOW_BORDER_WIDTH - 1);
		}

		/* 发送窗口获得焦点事件 */
		if (new_window && new_window->task) {
			EVENT event = { .window = new_window, .id = EVENT_WINDOW_GOTFOCUS };
			fifo_push_event(&new_window->task->fifo, &event);
		}
	}
}

/*
	@brief 激活窗口（将窗口置于顶层并获得焦点）。
	@param window 指向窗口结构的指针
*/
void window_activate(WINDOW *window)
{
	if (!window)
		return;
	LAYER *layer = window->layer;
	if (!layer)
		return;

	int32_t target_z;
	if (window->style & WINSTYLE_BOTTOMMOST) {
		target_z = 1;
	} else {
		target_z = g_lm.top; /* TOPMOST 窗口直接置于顶层 */
		if (!(window->style & WINSTYLE_TOPMOST)) {
			 /* 寻找非 TOPMOST 窗口 */
			for (int32_t z = g_lm.top; z > 0; z--)
				if (g_lm.layers[z]->window) {
					if (g_lm.layers[z]->window->style & WINSTYLE_TOPMOST)
						target_z--;
					else
						break;
				}

			if (target_z < 1)
				target_z = 1;
		}
	}

	layer_set_z(layer, target_z);
	layer_set_z(cursor_layer, g_lm.top);
	window_focus(&layer_focused, layer);
}

/*
	@brief 隐藏窗口（将窗口置于底层并失去焦点）。
	@param window 指向窗口结构的指针
*/
void window_inactivate(WINDOW *window)
{
	if (!window)
		return;
	LAYER *layer = window->layer;
	if (!layer)
		return;

	if (layer->z >= 0) {
		/* 隐藏目标图层 */
		layer_set_z(layer, -1);

		/* 重设焦点 */
		if (layer_focused == layer) {
			LAYER *new_focus = NULL;
			for (int32_t z = g_lm.top - 1; z >= 0; z--) {
				LAYER *candidate = g_lm.layers[z];
				if (candidate && candidate->window) {
					new_focus = candidate;
					break;
				}
			}
			window_focus(&layer_focused, new_focus);
		}
	}
}

/*
	@brief 设置窗口位置。
	@param window 指向窗口结构的指针
	@param x 新的 X 坐标
	@param y 新的 Y 坐标
*/
void window_set_position(WINDOW *window, int16_t x, int16_t y)
{
	if (!window)
		return;
	LAYER *layer = window->layer;
	if (!layer)
		return;

	window->x = x;
	window->y = y;
	layer_move(layer, x, y);
}

/*
	@brief 获取级联窗口位置。
	@param window 指向窗口结构的指针
	@param x 用于存储 X 坐标的指针
	@param y 用于存储 Y 坐标的指针
*/
void window_get_cascade_position(int16_t *x, int16_t *y, uint16_t width, uint16_t height)
{
#define CASCADE_OFFSET_X 24
#define CASCADE_OFFSET_Y 24

	static int16_t last_x = 0, last_y = 0;
	int32_t new_x, new_y;
	int32_t max_x, max_y;

	/* 基于上一次窗口位置计算候选坐标 */
	new_x = last_x + CASCADE_OFFSET_X;
	new_y = last_y + CASCADE_OFFSET_Y;

	/* 允许的最大横坐标 */
	max_x = (int32_t)g_fb.width - width - CASCADE_OFFSET_X;
	/* 允许的最大纵坐标 */
	max_y = (int32_t)g_fb.height - height - CASCADE_OFFSET_Y;

	/* 窗口尺寸过大导致无法满足边界要求，允许贴边显示 */
	if (max_x < 0) max_x = 0;
	if (max_y < 0) max_y = 0;

	if (new_x > max_x) new_x = CASCADE_OFFSET_X;
	if (new_y > max_y) new_y = CASCADE_OFFSET_Y;

	if (new_x < 0) new_x = 0;
	if (new_y < 0) new_y = 0;
	if (new_x > max_x) new_x = max_x;
	if (new_y > max_y) new_y = max_y;

	*x = (int16_t)new_x;
	*y = (int16_t)new_y;

	last_x = *x;
	last_y = *y;

#undef CASCADE_OFFSET_X
#undef CASCADE_OFFSET_Y
}

/*
	@brief 窗口命中测试。
	@param window 指向窗口结构的指针
	@param x 相对于窗口左上角的 X 坐标
	@param y 相对于窗口左上角的 Y 坐标
	@return 命中测试结果
*/
HIT_RESULT window_hit_test(WINDOW *window, int16_t x, int16_t y)
{
	if (!window)
		return HIT_NONE;

	if (x < 0 || x >= window->width || y < 0 || y >= window->height)
		return HIT_NONE; /* 不在窗口内 */

	if (!(window->style & WINSTYLE_SYSCONTROL))
		return HIT_CLIENT; /* 没有系统控件，全部区域都是客户区 */

	/* 边框区域检查 */
	if (x < WINDOW_BORDER_WIDTH && y < WINDOW_BORDER_WIDTH)
		return HIT_BORDER_TOPLEFT;
	if (x >= window->width - WINDOW_BORDER_WIDTH && y < WINDOW_BORDER_WIDTH)
		return HIT_BORDER_TOPRIGHT;
	if (x < WINDOW_BORDER_WIDTH && y >= window->height - WINDOW_BORDER_WIDTH)
		return HIT_BORDER_BOTTOMLEFT;
	if (x >= window->width - WINDOW_BORDER_WIDTH && y >= window->height - WINDOW_BORDER_WIDTH)
		return HIT_BORDER_BOTTOMRIGHT;
	if (x < WINDOW_BORDER_WIDTH)
		return HIT_BORDER_LEFT;
	if (x >= window->width - WINDOW_BORDER_WIDTH)
		return HIT_BORDER_RIGHT;
	if (y < WINDOW_BORDER_WIDTH)
		return HIT_BORDER_TOP;
	if (y >= window->height - WINDOW_BORDER_WIDTH)
		return HIT_BORDER_BOTTOM;

	/* 按钮区域检查 */
	if (window->style & WINSTYLE_MINIMIZABLE) {
		if (x >= window->width - 6 * DEFAULT_FONT_WIDTH - 2 * WINDOW_BORDER_WIDTH &&
			x < window->width - 3 * DEFAULT_FONT_WIDTH - 2 * WINDOW_BORDER_WIDTH &&
			y >= WINDOW_TITLETEXT_YPOS &&
			y < WINDOW_TITLETEXT_YPOS + DEFAULT_FONT_HEIGHT)
			return HIT_MINIMIZE_BUTTON;
	}
	if (x >= window->width - 3 * DEFAULT_FONT_WIDTH - 2 * WINDOW_BORDER_WIDTH &&
		x < window->width - 2 * WINDOW_BORDER_WIDTH &&
		y >= WINDOW_TITLETEXT_YPOS &&
		y < WINDOW_TITLETEXT_YPOS + DEFAULT_FONT_HEIGHT)
		return HIT_CLOSE_BUTTON;

	/* 图标区域检查 */
	if ((window->style & WINSTYLE_SYSCONTROL && window->style & WINSTYLE_ICON) &&
		x >= WINDOW_BORDER_WIDTH + 1 &&
		x < WINDOW_BORDER_WIDTH + 1 + WINDOW_ICON_WIDTH &&
		y >= WINDOW_TITLETEXT_YPOS &&
		y < WINDOW_TITLETEXT_YPOS + DEFAULT_FONT_HEIGHT)
		return HIT_ICON;

	/* 标题栏检查 */
	if (y >= WINDOW_BORDER_WIDTH && y < WINDOW_BORDER_WIDTH + WINDOW_TITLEBAR_HEIGHT)
		return HIT_TITLEBAR;

	return HIT_CLIENT; /* 其他区域都是客户区 */
}
