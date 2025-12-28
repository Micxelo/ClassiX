/*
	ui/widgets/widget.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/font.h>
#include <ClassiX/graphic.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>
#include <ClassiX/widgets.h>

void wd_draw_convex_string(LAYER *layer, int32_t x, int32_t y, COLOR fg_color, COLOR bg_color, const char *str, const BITMAP_FONT *font)
{
	/* 绘制阴影 */
	draw_unicode_string(layer->buf, layer->width, x + 1, y + 1, bg_color, str, font);
	
	/* 绘制前景文字 */
	draw_unicode_string(layer->buf, layer->width, x, y, fg_color, str, font);
}
