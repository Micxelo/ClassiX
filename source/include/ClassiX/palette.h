/*
	include/ClassiX/palette.h
*/

#ifndef _CLASSIX_PALETTE_H_
#define _CLASSIX_PALETTE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef union {
	uint32_t color;
	struct {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
} COLOR;

#define COLOR32(_color)						((COLOR) { .color = (_color) })
#define COLOR32_FROM_ARGB(_r, _g, _b, _a)	((COLOR) { .a = (_a), .r = (_r), .g = (_g), .b = (_b) })

#define COLOR32_INVERT(_color)				\
	((COLOR) { .a = (_color).a, .r = 255 - (_color).r, .g = 255 - (_color).g, .b = 255 - (_color).b })

#define SET_PIXEL32(buf, bx, x, y, c)		((buf)[(y) * (bx) + (x)] = (c).color)
#define GET_PIXEL32(buf, bx, x, y)			((COLOR) { .color = (buf)[(y) * (bx) + (x)] })

#define COLOR_CGA_BLACK						COLOR32(0xff000000)	/* CGA 黑 */
#define COLOR_CGA_BLUE						COLOR32(0xff0000aa)	/* CGA 蓝 */
#define COLOR_CGA_GREEN						COLOR32(0xff00aa00)	/* CGA 绿 */
#define COLOR_CGA_CYAN						COLOR32(0xff00aaaa)	/* CGA 青 */
#define COLOR_CGA_RED						COLOR32(0xffaa0000)	/* CGA 红 */
#define COLOR_CGA_MAGENTA					COLOR32(0xffaa00aa)	/* CGA 品红 */
#define COLOR_CGA_BROWN						COLOR32(0xffaa5500)	/* CGA 棕 */
#define COLOR_CGA_LIGHTGRAY					COLOR32(0xffaaaaaa)	/* CGA 浅灰 */
#define COLOR_CGA_DARKGRAY					COLOR32(0xff555555)	/* CGA 暗灰 */
#define COLOR_CGA_LIGHTBLUE					COLOR32(0xff5555ff)	/* CGA 浅蓝 */
#define COLOR_CGA_LIGHTGREEN				COLOR32(0xff55ff55)	/* CGA 浅绿 */
#define COLOR_CGA_LIGHTCYAN					COLOR32(0xff55ffff)	/* CGA 浅青 */
#define COLOR_CGA_LIGHTRED					COLOR32(0xffff5555)	/* CGA 浅红 */
#define COLOR_CGA_LIGHTMAGENTA				COLOR32(0xffff55ff)	/* CGA 浅品红 */
#define COLOR_CGA_YELLOW					COLOR32(0xffffff55)	/* CGA 黄 */
#define COLOR_CGA_WHITE						COLOR32(0xffffffff)	/* CGA 白 */

#define COLOR_ALICEBLUE						COLOR32(0xfff0f8ff)	/* 爱丽丝蓝 */
#define COLOR_ANTIQUEWHITE					COLOR32(0xfffaebd7)	/* 古董白 */
#define COLOR_AQUA							COLOR32(0xff00ffff)	/* 水色 */
#define COLOR_AQUAMARINE					COLOR32(0xff7fffd4)	/* 碧绿色 */
#define COLOR_AZURE							COLOR32(0xfff0ffff)	/* 天蓝色 */
#define COLOR_BEIGE							COLOR32(0xfff5f5dc)	/* 米黄色 */
#define COLOR_BISQUE						COLOR32(0xffffe4c4)	/* 陶坯黄 */
#define COLOR_BLACK							COLOR32(0xff000000)	/* 黑色 */
#define COLOR_BLANCHEDALMOND				COLOR32(0xffffebcd)	/* 杏仁白 */
#define COLOR_BLUE							COLOR32(0xff0000ff)	/* 蓝色 */
#define COLOR_BLUEVIOLET					COLOR32(0xff8a2be2)	/* 蓝紫罗兰 */
#define COLOR_BROWN							COLOR32(0xffa52a2a)	/* 褐色 */
#define COLOR_BURLYWOOD						COLOR32(0xffdeb887)	/* 硬木色 */
#define COLOR_CADETBLUE						COLOR32(0xff5f9ea0)	/* 军服蓝 */
#define COLOR_CHARTREUSE					COLOR32(0xff7fff00)	/* 查特酒绿 */
#define COLOR_CHOCOLATE						COLOR32(0xffd2691e)	/* 巧克力色 */
#define COLOR_CORAL							COLOR32(0xffff7f50)	/* 珊瑚红 */
#define COLOR_CORNFLOWERBLUE				COLOR32(0xff6495ed)	/* 矢车菊蓝 */
#define COLOR_CORNSILK						COLOR32(0xfffff8dc)	/* 玉米丝色 */
#define COLOR_CRIMSON						COLOR32(0xffdc143c)	/* 绯红 */
#define COLOR_CYAN							COLOR32(0xff00ffff)	/* 青色 */
#define COLOR_DARKBLUE						COLOR32(0xff00008b)	/* 暗蓝色 */
#define COLOR_DARKCYAN						COLOR32(0xff008b8b)	/* 暗青色 */
#define COLOR_DARKGOLDENROD					COLOR32(0xffb8860b)	/* 暗金菊色 */
#define COLOR_DARKGRAY						COLOR32(0xffa9a9a9)	/* 暗灰色 */
#define COLOR_DARKGREEN						COLOR32(0xff006400)	/* 暗绿色 */
#define COLOR_DARKKHAKI						COLOR32(0xffbdb76b)	/* 暗卡其色 */
#define COLOR_DARKMAGENTA					COLOR32(0xff8b008b)	/* 暗洋红色 */
#define COLOR_DARKOLIVEGREEN				COLOR32(0xff556b2f)	/* 暗橄榄绿 */
#define COLOR_DARKORANGE					COLOR32(0xffff8c00)	/* 暗橙色 */
#define COLOR_DARKORCHID					COLOR32(0xff9932cc)	/* 暗兰花紫 */
#define COLOR_DARKRED						COLOR32(0xff8b0000)	/* 暗红色 */
#define COLOR_DARKSALMON					COLOR32(0xffe9967a)	/* 暗鲑红 */
#define COLOR_DARKSEAGREEN					COLOR32(0xff8fbc8f)	/* 暗海绿 */
#define COLOR_DARKSLATEBLUE					COLOR32(0xff483d8b)	/* 暗岩蓝 */
#define COLOR_DARKSLATEGRAY					COLOR32(0xff2f4f4f)	/* 暗岩灰 */
#define COLOR_DARKTURQUOISE					COLOR32(0xff00ced1)	/* 暗松石绿 */
#define COLOR_DARKVIOLET					COLOR32(0xff9400d3)	/* 暗紫罗兰 */
#define COLOR_DEEPPINK						COLOR32(0xffff1493)	/* 深粉红色 */
#define COLOR_DEEPSKYBLUE					COLOR32(0xff00bfff)	/* 深天蓝 */
#define COLOR_DIMGRAY						COLOR32(0xff696969)	/* 昏灰色 */
#define COLOR_DODGERBLUE					COLOR32(0xff1e90ff)	/* 道奇蓝 */
#define COLOR_FIREBRICK						COLOR32(0xffb22222)	/* 耐火砖红 */
#define COLOR_FLORALWHITE					COLOR32(0xfffffaf0)	/* 花卉白 */
#define COLOR_FORESTGREEN					COLOR32(0xff228b22)	/* 森林绿 */
#define COLOR_FUCHSIA						COLOR32(0xffff00ff)	/* 洋红色 */
#define COLOR_GAINSBORO						COLOR32(0xffdcdcdc)	/* 庚斯博罗灰 */
#define COLOR_GHOSTWHITE					COLOR32(0xfff8f8ff)	/* 幽灵白 */
#define COLOR_GOLD							COLOR32(0xffffd700)	/* 金色 */
#define COLOR_GOLDENROD						COLOR32(0xffdaa520)	/* 金菊色 */
#define COLOR_GRAY							COLOR32(0xff808080)	/* 灰色 */
#define COLOR_GREEN							COLOR32(0xff008000)	/* 绿色 */
#define COLOR_GREENYELLOW					COLOR32(0xffadff2f)	/* 黄绿色 */
#define COLOR_HONEYDEW						COLOR32(0xfff0fff0)	/* 蜜瓜绿 */
#define COLOR_HOTPINK						COLOR32(0xffff69b4)	/* 暖粉色 */
#define COLOR_INDIANRED						COLOR32(0xffcd5c5c)	/* 印度红 */
#define COLOR_INDIGO						COLOR32(0xff4b0082)	/* 靛蓝色 */
#define COLOR_IVORY							COLOR32(0xfffffff0)	/* 象牙白 */
#define COLOR_KHAKI							COLOR32(0xfff0e68c)	/* 卡其色 */
#define COLOR_LAVENDER						COLOR32(0xffe6e6fa)	/* 薰衣草紫 */
#define COLOR_LAVENDERBLUSH					COLOR32(0xfffff0f5)	/* 薰衣草紫红 */
#define COLOR_LAWNGREEN						COLOR32(0xff7cfc00)	/* 草坪绿 */
#define COLOR_LEMONCHIFFON					COLOR32(0xfffffacd)	/* 柠檬绸色 */
#define COLOR_LIGHTBLUE						COLOR32(0xffadd8e6)	/* 亮蓝色 */
#define COLOR_LIGHTCORAL					COLOR32(0xfff08080)	/* 亮珊瑚红 */
#define COLOR_LIGHTCYAN						COLOR32(0xffe0ffff)	/* 亮青色 */
#define COLOR_LIGHTGOLDENRODYELLOW			COLOR32(0xfffafad2)	/* 亮金菊黄 */
#define COLOR_LIGHTGRAY						COLOR32(0xffd3d3d3)	/* 亮灰色 */
#define COLOR_LIGHTGREEN					COLOR32(0xff90ee90)	/* 亮绿色 */
#define COLOR_LIGHTPINK						COLOR32(0xffffb6c1)	/* 亮粉红色 */
#define COLOR_LIGHTSALMON					COLOR32(0xffffa07a)	/* 亮鲑红 */
#define COLOR_LIGHTSEAGREEN					COLOR32(0xff20b2aa)	/* 亮海绿 */
#define COLOR_LIGHTSKYBLUE					COLOR32(0xff87cefa)	/* 亮天蓝 */
#define COLOR_LIGHTSLATEGRAY				COLOR32(0xff778899)	/* 亮岩灰 */
#define COLOR_LIGHTSTEELBLUE				COLOR32(0xffb0c4de)	/* 亮钢蓝 */
#define COLOR_LIGHTYELLOW					COLOR32(0xffffffe0)	/* 亮黄色 */
#define COLOR_LIME							COLOR32(0xff00ff00)	/* 鲜绿色 */
#define COLOR_LIMEGREEN						COLOR32(0xff32cd32)	/* 酸橙绿 */
#define COLOR_LINEN							COLOR32(0xfffaf0e6)	/* 亚麻色 */
#define COLOR_MAGENTA						COLOR32(0xffff00ff)	/* 洋红色 */
#define COLOR_MAROON						COLOR32(0xff800000)	/* 栗色 */
#define COLOR_MEDIUMAQUAMARINE				COLOR32(0xff66cdaa)	/* 中碧绿色 */
#define COLOR_MEDIUMBLUE					COLOR32(0xff0000cd)	/* 中蓝色 */
#define COLOR_MEDIUMORCHID					COLOR32(0xffba55d3)	/* 中兰花紫 */
#define COLOR_MEDIUMPURPLE					COLOR32(0xff9370db)	/* 中紫色 */
#define COLOR_MEDIUMSEAGREEN				COLOR32(0xff3cb371)	/* 中海绿 */
#define COLOR_MEDIUMSLATEBLUE				COLOR32(0xff7b68ee)	/* 中岩蓝 */
#define COLOR_MEDIUMSPRINGGREEN				COLOR32(0xff00fa9a)	/* 中春绿 */
#define COLOR_MEDIUMTURQUOISE				COLOR32(0xff48d1cc)	/* 中松石绿 */
#define COLOR_MEDIUMVIOLETRED				COLOR32(0xffc71585)	/* 中紫罗兰红 */
#define COLOR_MIDNIGHTBLUE					COLOR32(0xff191970)	/* 午夜蓝 */
#define COLOR_MIKU							COLOR32(0xff39c5bb)	/* 初音绿 */
#define COLOR_MINTCREAM						COLOR32(0xfff5fffa)	/* 薄荷奶油色 */
#define COLOR_MISTYROSE						COLOR32(0xffffe4e1)	/* 雾玫瑰色 */
#define COLOR_MOCCASIN						COLOR32(0xffffe4b5)	/* 鹿皮鞋色 */
#define COLOR_NAVAJOWHITE					COLOR32(0xffffdead)	/* 那瓦霍白 */
#define COLOR_NAVY							COLOR32(0xff000080)	/* 海军蓝 */
#define COLOR_OLDLACE						COLOR32(0xfffdf5e6)	/* 旧蕾丝色 */
#define COLOR_OLIVE							COLOR32(0xff808000)	/* 橄榄色 */
#define COLOR_OLIVEDRAB						COLOR32(0xff6b8e23)	/* 橄榄褐 */
#define COLOR_ORANGE						COLOR32(0xffffa500)	/* 橙色 */
#define COLOR_ORANGERED						COLOR32(0xffff4500)	/* 橙红色 */
#define COLOR_ORCHID						COLOR32(0xffda70d6)	/* 兰花紫 */
#define COLOR_PALEGOLDENROD					COLOR32(0xffeee8aa)	/* 苍金菊色 */
#define COLOR_PALEGREEN						COLOR32(0xff98fb98)	/* 苍绿色 */
#define COLOR_PALETURQUOISE					COLOR32(0xffafeeee)	/* 苍松石绿 */
#define COLOR_PALEVIOLETRED					COLOR32(0xffdb7093)	/* 苍紫罗兰红 */
#define COLOR_PAPAYAWHIP					COLOR32(0xffffefd5)	/* 番木瓜色 */
#define COLOR_PEACHPUFF						COLOR32(0xffffdab9)	/* 桃色 */
#define COLOR_PERU							COLOR32(0xffcd853f)	/* 秘鲁色 */
#define COLOR_PINK							COLOR32(0xffffc0cb)	/* 粉红色 */
#define COLOR_PLUM							COLOR32(0xffdda0dd)	/* 梅红色 */
#define COLOR_POWDERBLUE					COLOR32(0xffb0e0e6)	/* 粉末蓝 */
#define COLOR_PURPLE						COLOR32(0xff800080)	/* 紫色 */
#define COLOR_RED							COLOR32(0xffff0000)	/* 红色 */
#define COLOR_ROSYBROWN						COLOR32(0xffbc8f8f)	/* 玫瑰褐 */
#define COLOR_ROYALBLUE						COLOR32(0xff4169e1)	/* 皇家蓝 */
#define COLOR_SADDLEBROWN					COLOR32(0xff8b4513)	/* 鞍褐 */
#define COLOR_SALMON						COLOR32(0xfffa8072)	/* 鲑红 */
#define COLOR_SANDYBROWN					COLOR32(0xfff4a460)	/* 沙褐 */
#define COLOR_SEAGREEN						COLOR32(0xff2e8b57)	/* 海绿 */
#define COLOR_SEASHELL						COLOR32(0xfffff5ee)	/* 海贝色 */
#define COLOR_SIENNA						COLOR32(0xffa0522d)	/* 赭黄 */
#define COLOR_SILVER						COLOR32(0xffc0c0c0)	/* 颜色 */
#define COLOR_SKYBLUE						COLOR32(0xff87ceeb)	/* 天蓝 */
#define COLOR_SLATEBLUE						COLOR32(0xff6a5acd)	/* 岩蓝 */
#define COLOR_SLATEGRAY						COLOR32(0xff708090)	/* 岩灰 */
#define COLOR_SNOW							COLOR32(0xfffffafa)	/* 雪色 */
#define COLOR_SPRINGGREEN					COLOR32(0xff00ff7f)	/* 春绿 */
#define COLOR_STEELBLUE						COLOR32(0xff4682b4)	/* 钢蓝 */
#define COLOR_TAN							COLOR32(0xffd2b48c)	/* 日晒色 */
#define COLOR_TEAL							COLOR32(0xff008080)	/* 凫蓝 */
#define COLOR_THISTLE						COLOR32(0xffd8bfd8)	/* 蓟紫 */
#define COLOR_TOMATO						COLOR32(0xffff6347)	/* 番茄红 */
#define COLOR_TRANSPARENT					COLOR32(0x00ffffff)	/* 透明色 */
#define COLOR_TURQUOISE						COLOR32(0xff40e0d0)	/* 松石绿 */
#define COLOR_VIOLET						COLOR32(0xffee82ee)	/* 紫罗兰 */
#define COLOR_WHEAT							COLOR32(0xfff5deb3)	/* 小麦色 */
#define COLOR_WHITE							COLOR32(0xffffffff)	/* 白色 */
#define COLOR_WHITESMOKE					COLOR32(0xfff5f5f5)	/* 白烟色 */
#define COLOR_YELLOW						COLOR32(0xffffff00)	/* 黄色 */
#define COLOR_YELLOWGREEN					COLOR32(0xff9acd32)	/* 黄绿色 */

COLOR alpha_blend(COLOR fg, COLOR bg);

#ifdef __cplusplus
	}
#endif

#endif
