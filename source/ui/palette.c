/*
	ui/palette.c
*/

#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

/*
	@brief 颜色混合。
	@param fg 前景色
	@param bg 背景色
	@return 混合后的颜色，Alpha = 255
	@note 背景色应完全不透明。
*/
inline COLOR alpha_blend(COLOR fg, COLOR bg)
{
	if (fg.a == 0x00)
		return bg;
	if (fg.a == 0xff)
		return fg;

	uint32_t alpha = fg.a + 1;
	uint32_t inv_alpha = 257 - alpha;

	uint32_t r = (fg.r * alpha + bg.r * inv_alpha) >> 8;
	uint32_t g = (fg.g * alpha + bg.g * inv_alpha) >> 8;
	uint32_t b = (fg.b * alpha + bg.b * inv_alpha) >> 8;

	return (COLOR) { .a = 0xff, .r = (uint8_t) r, .g = (uint8_t) g, .b = (uint8_t) b };
}
