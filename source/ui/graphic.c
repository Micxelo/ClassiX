/*
	ui/graphic.c
*/

#include <ClassiX/font.h>
#include <ClassiX/graphic.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

/*
	@brief 指定左上角坐标、宽度和高度绘制矩形边框。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x 左上角 x 坐标
	@param y 左上角 y 坐标
	@param width 矩形宽度
	@param height 矩形高度
	@param thickness 边框厚度
	@param color 边框颜色
*/
void draw_rectangle(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t thickness, COLOR color)
{
	draw_rectangle_by_corners(buf, bx, x, y, x + width - 1, y + height - 1, thickness, color);
}

/*
	@brief 指定两个角点坐标绘制矩形边框。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 左上角 x 坐标
	@param y0 左上角 y 坐标
	@param x1 右下角 x 坐标
	@param y1 右下角 y 坐标
	@param thickness 边框厚度
	@param color 边框颜色
*/
void draw_rectangle_by_corners(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t thickness, COLOR color)
{
	if (thickness == 0) return;
	
	/* 确保坐标顺序正确 */
	if (x0 > x1) {
		uint32_t temp = x0;
		x0 = x1;
		x1 = temp;
	}
	if (y0 > y1) {
		uint32_t temp = y0;
		y0 = y1;
		y1 = temp;
	}

	if (thickness == 1) {
		draw_line(buf, bx, x0, y0, x1, y0, color); /* 上边 */
		draw_line(buf, bx, x0, y1, x1, y1, color); /* 下边 */
		draw_line(buf, bx, x0, y0, x0, y1, color); /* 左边 */
		draw_line(buf, bx, x1, y0, x1, y1, color); /* 右边 */
	} else {
		fill_rectangle_by_corners(buf, bx, x0, y0, x1, y0 + thickness - 1, color); /* 上边 */
		fill_rectangle_by_corners(buf, bx, x0, y1 - thickness + 1, x1, y1, color); /* 下边 */
		fill_rectangle_by_corners(buf, bx, x0, y0, x0 + thickness - 1, y1, color); /* 左边 */
		fill_rectangle_by_corners(buf, bx, x1 - thickness + 1, y0, x1, y1, color); /* 右边 */
	}
}


/*
	@brief 指定一点坐标、宽度和高度填充矩形区域。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 左上角 x 坐标
	@param y0 左上角 y 坐标
	@param width 矩形宽度
	@param height 矩形高度
	@param color 填充颜色
*/
void fill_rectangle(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, uint16_t width, uint16_t height, COLOR color)
{
	fill_rectangle_by_corners(buf, bx, x, y, x + width - 1, y + height - 1, color);
}

/*
	@brief 指定两个角点坐标填充矩形区域。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 左上角 x 坐标
	@param y0 左上角 y 坐标
	@param x1 右下角 x 坐标
	@param y1 右下角 y 坐标
	@param color 填充颜色
*/
void fill_rectangle_by_corners(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color)
{
	/* 确保坐标顺序正确 */
	if (x0 > x1) {
		uint32_t temp = x0;
		x0 = x1;
		x1 = temp;
	}
	if (y0 > y1) {
		uint32_t temp = y0;
		y0 = y1;
		y1 = temp;
	}

	uint32_t x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++) {
			SET_PIXEL32(buf, bx, x, y, color);
		}
	}
}

/*
	@brief 指定两点坐标绘制直线。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 起点 x 坐标
	@param y0 起点 y 坐标
	@param x1 终点 x 坐标
	@param y1 终点 y 坐标
	@param color 线段颜色
*/
void draw_line(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color)
{
	int32_t dx = x1 - x0;
	int32_t dy = y1 - y0;
	int32_t abs_dx = (dx < 0) ? -dx : dx;
	int32_t abs_dy = (dy < 0) ? -dy : dy;

	int32_t sx = (dx < 0) ? -1 : 1;
	int32_t sy = (dy < 0) ? -1 : 1;

	if (abs_dx >= abs_dy) {
		int32_t err = abs_dx / 2;
		while (x0 != x1) {
			SET_PIXEL32(buf, bx, x0, y0, color);
			err -= abs_dy;
			if (err < 0) {
				y0 += sy;
				err += abs_dx;
			}
			x0 += sx;
		}
	} else {
		int32_t err = abs_dy / 2;
		while (y0 != y1) {
			SET_PIXEL32	(buf, bx, x0, y0, color);
			err -= abs_dx;
			if (err < 0) {
				x0 += sx;
				err += abs_dy;
			}
			y0 += sy;
		}
	}
}

/* 绘制当前点的 8 个对称位置 */
static void _draw_circle_points(uint32_t *buf, uint16_t bx, int32_t x0, int32_t y0, int32_t x, int32_t y, COLOR color)
{
	SET_PIXEL32(buf, bx, x0 + x, y0 + y, color);
	SET_PIXEL32(buf, bx, x0 - x, y0 + y, color);
	SET_PIXEL32(buf, bx, x0 + x, y0 - y, color);
	SET_PIXEL32(buf, bx, x0 - x, y0 - y, color);
	SET_PIXEL32(buf, bx, x0 + y, y0 + x, color);
	SET_PIXEL32(buf, bx, x0 - y, y0 + x, color);
	SET_PIXEL32(buf, bx, x0 + y, y0 - x, color);
	SET_PIXEL32(buf, bx, x0 - y, y0 - x, color);
}

/*
	@brief 指定圆心坐标和半径绘制圆形。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 圆心 x 坐标
	@param y0 圆心 y 坐标
	@param radius 圆半径
	@param color 圆形颜色
*/
void draw_circle(uint32_t *buf, uint16_t bx, int32_t x0, int32_t y0, int32_t radius, COLOR color)
{
	int32_t x = 0;
	int32_t y = radius;
	int32_t d = 1 - radius;

	/* 绘制初始的 8 个对称点 */
	_draw_circle_points(buf, bx, x0, y0, x, y, color);

	while (x < y) {
		x++;
		if (d < 0) {
			d += 2 * x + 1; /* 选择上方的点 */ 
		} else {
			y--; /* 选择右下方的点 */
			d += 2 * (x - y) + 1;
		}
		/* 绘制当前点的 8 个对称位置 */
		_draw_circle_points(buf, bx, x0, y0, x, y, color);
	}
}

/*
	@brief 指定圆心坐标和半径填充圆形。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 圆心 x 坐标
	@param y0 圆心 y 坐标
	@param radius 圆半径
	@param color 填充颜色
*/
void fill_circle(uint32_t *buf, uint16_t bx, int32_t x0, int32_t y0, int32_t radius, COLOR color)
{
	for (int32_t y = 0; y <= radius; y++) {
		for (int32_t x = 0; x <= radius; x++) {
			if (x * x + y * y <= radius * radius) {
				SET_PIXEL32(buf, bx, x0 + x, y0 + y, color);
				SET_PIXEL32(buf, bx, x0 - x, y0 + y, color);
				SET_PIXEL32(buf, bx, x0 + x, y0 - y, color);
				SET_PIXEL32(buf, bx, x0 - x, y0 - y, color);
			}
		}
	}
}

static void _draw_ellipse_points(uint32_t *buf, uint16_t bx, int32_t x0, int32_t y0, int32_t x, int32_t y, COLOR color)
{
	/* 绘制当前点的 4 个对称位置 */
	SET_PIXEL32(buf, bx, x0 + x, y0 + y, color);
	SET_PIXEL32(buf, bx, x0 - x, y0 + y, color);
	SET_PIXEL32(buf, bx, x0 + x, y0 - y, color);
	SET_PIXEL32(buf, bx, x0 - x, y0 - y, color);
}

/*
	@brief 指定椭圆中心坐标和长短轴绘制椭圆。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 椭圆中心 x 坐标
	@param y0 椭圆中心 y 坐标
	@param a 椭圆长轴长度
	@param b 椭圆短轴长度
	@param color 椭圆颜色
	@note 规定长轴为 x 轴，短轴为 y 轴。
*/
void draw_ellipse(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color)
{
	if (a == 0 || b == 0)
		return;

	int32_t x = 0, y = b;
	int32_t a2 = a * a, b2 = b * b;
	int32_t d1 = b2 - a2 * b + a2 / 4;

	while (b2 * x <= a2 * y) {
		_draw_ellipse_points(buf, bx, x0, y0, x, y, color);
		x++;
		if (d1 < 0) {
			d1 += 2 * b2 * x + b2;
		} else {
			y--;
			d1 += 2 * b2 * x - 2 * a2 * y + b2;
		}
	}

	int32_t d2 = b2 * (x + 0.5) * (x + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
	while (y >= 0) {
		_draw_ellipse_points(buf, bx, x0, y0, x, y, color);
		y--;
		if (d2 > 0) {
			d2 += -2 * a2 * y + a2;
		} else {
			x++;
			d2 += 2 * b2 * x - 2 * a2 * y + a2;
		}
	}
}

/*
	@brief 指定椭圆中心坐标和长短轴填充椭圆。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x0 椭圆中心 x 坐标
	@param y0 椭圆中心 y 坐标
	@param a 椭圆长轴长度
	@param b 椭圆短轴长度
	@param color 填充颜色
	@note 规定长轴为 x 轴，短轴为 y 轴。
*/
void fill_ellipse(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color)
{
	for (int32_t y = 0; y <= b; y++) {
		for (int32_t x = 0; x <= a; x++) {
			if ((x * x * b * b + y * y * a * a) <= (a * a * b * b)) {
				SET_PIXEL32(buf, bx, x0 + x, y0 + y, color);
				SET_PIXEL32(buf, bx, x0 - x, y0 + y, color);
				SET_PIXEL32(buf, bx, x0 + x, y0 - y, color);
				SET_PIXEL32(buf, bx, x0 - x, y0 - y, color);
			}
		}
	}
}

/*
	@brief 位块拷贝。
	@param src 源缓冲区
	@param src_bx 源缓冲区的宽度
	@param src_x 源缓冲区左上角 x 坐标
	@param src_y 源缓冲区左上角 y 坐标
	@param width 拷贝宽度
	@param height 拷贝高度
	@param dst 目标缓冲区
	@param dst_bx 目标缓冲区的宽度
	@param dst_x 目标缓冲区左上角 x 坐标
	@param dst_y 目标缓冲区左上角 y 坐标
*/
void bit_blit(const uint32_t *src, uint16_t src_bx, uint16_t src_x, uint16_t src_y, uint16_t width, uint16_t height,
	uint32_t *dst, uint16_t dst_bx, uint16_t dst_x, uint16_t dst_y)
{
	for (uint16_t y = 0; y < height; y++)
		for (uint16_t x = 0; x < width; x++)
			SET_PIXEL32(dst, dst_bx, dst_x + x, dst_y + y, GET_PIXEL32(src, src_bx, src_x + x, src_y + y));
}

/* 绘制单个字形 */
static void draw_font_char(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, COLOR color, 
						   const uint8_t *font_data, uint16_t width, uint16_t height)
{
	uint32_t bytes_per_row = (width + 7) / 8;

	for (uint32_t row = 0; row < height; row++) {
		const uint8_t *row_data = font_data + row * bytes_per_row;
		
		for (uint32_t col = 0; col < width; col++) {
			uint32_t byte_index = col / 8;
			uint32_t bit_index = 7 - (col % 8);  /* PSF 格式是高位在前 */
			
			if (row_data[byte_index] & (1 << bit_index))
				SET_PIXEL32(buf, bx, x + col, y + row, color);
		}
	}
}

/*
	@brief 使用 PSF 字体绘制 ASCII 字符串。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x X 坐标
	@param y Y 坐标
	@param color 字符颜色
	@param str 字符串内容
	@param font 使用的 PSF 字体
	@note 仅支持单行、ASCII 字符集。
*/
void draw_ascii_string(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, COLOR color, const char *str, const BITMAP_FONT *font)
{
	for (; *str != 0; str++) {
		uint32_t char_index = psf_find_char_index(font, (uint8_t) *str);
		const uint8_t *char_data = font->buf + char_index * font->charsize;
		draw_font_char(buf, bx, x, y, color, char_data, font->width, font->height);
		x += font->width;
	}
}

/*
	@brief 使用 PSF 字体绘制 Unicode 字符串。
	@param buf 绘图缓冲区
	@param bx 绘图缓冲区的宽度
	@param x X 坐标
	@param y Y 坐标
	@param color 字符颜色
	@param str 字符串内容
	@param font 使用的 PSF 字体
*/
void draw_unicode_string(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, COLOR color, const char *str, const BITMAP_FONT *font)
{
	uint16_t current_x = x;

	while (*str != 0x00) {
		uint32_t unicode = 0;
		uint8_t first_byte = (uint8_t)*str;
		int32_t bytes_to_skip = 1;
		
		/* 解码 UTF-8 */
		if ((first_byte & 0x80) == 0) {
			/* 单字节 UTF-8 (ASCII) */
			unicode = first_byte;
		} else if ((first_byte & 0xE0) == 0xC0) {
			/* 双字节 UTF-8 */
			if ((str[1] & 0xC0) == 0x80) {
				unicode = ((first_byte & 0x1F) << 6) | (str[1] & 0x3F);
				bytes_to_skip = 2;
			}
		} else if ((first_byte & 0xF0) == 0xE0) {
			/* 三字节 UTF-8 */
			if ((str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80) {
				unicode = ((first_byte & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
				bytes_to_skip = 3;
			}
		} else if ((first_byte & 0xF8) == 0xF0) {
			/* 四字节 UTF-8 */
			if ((str[1] & 0xC0) == 0x80 && (str[2] & 0xC0) == 0x80 && (str[3] & 0xC0) == 0x80) {
				unicode = ((first_byte & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
				bytes_to_skip = 4;
			}
		}
		
		if (unicode == 0) {
			/* 无效的 UTF-8，跳过当前字节 */
			str++;
			continue;
		}
		
		/* 查找字符索引 */
		uint32_t char_index = psf_find_char_index(font, unicode);
		
		/* 绘制字符 */
		const uint8_t *char_data = font->buf + char_index * font->charsize;
		draw_font_char(buf, bx, current_x, y, color, char_data, font->width, font->height);
		
		current_x += font->width;
		str += bytes_to_skip;
	}
}
