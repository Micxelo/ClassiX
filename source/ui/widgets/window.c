/*
	ui/widgets/window.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/fifo.h>
#include <ClassiX/font.h>
#include <ClassiX/graphic.h>
#include <ClassiX/layer.h>
#include <ClassiX/memory.h>
#include <ClassiX/palette.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>
#include <ClassiX/widgets.h>

#define WINDOW_EVENT_QUEUE_SIZE				128	/* 窗口事件队列大小 */

/*
	@brief 绘制窗口。
	@param window 指向窗口结构的指针
*/
static void _window_draw(WINDOW *window)
{
	/* 绘制窗口背景 */
	fill_rectangle(window->layer->buf, window->base.width, 0, 0, window->base.width, window->base.height, COLOR_SYSTEM_WIDGET_BACKGROUND);

	if (window->base.style & WINSTYLE_SYSCONTROL) {
		/* 绘制边框 */
		draw_rectangle(window->layer->buf, window->base.width,
			0, 0,
			window->base.width, window->base.height,
			1,
			COLOR_SYSTEM_WIDGET_FRAME);

		/* 绘制标题栏 */
		fill_rectangle(window->layer->buf, window->base.width,
			1, 1,
			window->base.width - 2, 18,
			COLOR_SYSTEM_WIDGET_TITLEBAR_ACTIVE);

		/* 绘制关闭按钮 */
		wd_draw_convex_string(window->layer,
			window->base.width - 24 - 2, 3,
			COLOR_SYSTEM_WIDGET_TEXT_ACTIVE,
			COLOR_SYSTEM_WIDGET_TEXT_INACTIVE,
			"[X]",
			&font_terminus_16b);

		if (window->base.style & WINSTYLE_MINIMIZABLE)
			/* 绘制最小化按钮 */
			wd_draw_convex_string(window->layer,
				window->base.width - 48 - 2, 3,
				COLOR_SYSTEM_WIDGET_TEXT_ACTIVE,
				COLOR_SYSTEM_WIDGET_TEXT_INACTIVE,
				"[-]",
				&font_terminus_16b);

		if (window->base.data)
			/* 绘制标题文字 */
			wd_draw_convex_string(window->layer,
				((window->base.style & WINSTYLE_ICON) ? 2 + 16 : 2), 3,
				COLOR_SYSTEM_WIDGET_TEXT_ACTIVE,
				COLOR_SYSTEM_WIDGET_TEXT_INACTIVE,
				(const char *) window->base.data,
				&font_terminus_16b);
	}
}

/*
	@brief 创建窗口。
	@param window 指向窗口结构的指针
	@param x 窗口的初始 X 坐标
	@param y 窗口的初始 Y 坐标
	@param width 窗口宽度
	@param height 窗口高度
	@param style 窗口样式
	@param title 窗口标题
	@param task 关联的任务
	@return 错误码
*/
int32_t window_create(WINDOW *window, int16_t x, int16_t y, uint16_t width, uint16_t height, uint32_t style, const char *title, TASK *task)
{
	if (!window)
		return WD_INVALID_PARAM; /* 参数无效 */

	window->base.type = WIDGET_TYPE_WINDOW;
	window->base.x = x;
	window->base.y = y;
	window->base.width = width;
	window->base.height = height;
	window->base.style = style;
	window->base.data = (void *) title;
	window->base.parent = NULL;
	window->base.first_child = NULL;
	window->base.next_sibling = NULL;
	window->base.process = NULL;

	/* 创建图层 */
	window->layer = layer_alloc(width, height, false);
	if (!window->layer)
		return WD_NO_MEMORY; /* 内存不足 */

	window->layer->window = window;

	/* 绘制窗口 */
	_window_draw(window);

	layer_move(window->layer, x, y);
	layer_set_z(window->layer, g_lm.top);

	return WD_SUCCESS;
}

/*
	@brief 销毁窗口。
	@param window 指向窗口结构的指针
	@return 错误码
*/
int32_t window_destroy(WINDOW *window)
{
	if (!window)
		return WD_INVALID_PARAM; /* 参数无效 */

	/* 释放图层 */
	layer_free(window->layer);
	window->layer = NULL;

	return WD_SUCCESS;
}
