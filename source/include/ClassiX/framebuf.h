/*
	include/ClassiX/framebuf.h
*/

#ifndef _CLASSIX_FRAMEBUF_H_
#define _CLASSIX_FRAMEBUF_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef uint32_t COLOR;

#define COLOR32(a, r, g, b)					\
	(COLOR) (((uint8_t) (a) << 24) | ((uint8_t) (r) << 16) | ((uint8_t) (g) << 8) | (uint8_t) (b))
#define COLOR32_ALPHA(c)					((uint8_t) ((c) >> 24))
#define COLOR32_RED(c)						((uint8_t) ((c) >> 16))
#define COLOR32_GREEN(c)					((uint8_t) ((c) >> 8))
#define COLOR32_BLUE(c)						((uint8_t) (c))

#define SET_PIXEL32(buf, bx, x, y, color)	((buf)[(y) * (bx) + (x)] = (color))
#define GET_PIXEL32(buf, bx, x, y)			((buf)[(y) * (bx) + (x)])

void draw_rectangle(COLOR *buf, uint16_t bx, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t thickness, COLOR color);
void draw_rectangle_by_corners(COLOR *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t thickness, COLOR color);
void fill_rectangle(COLOR *buf, uint16_t bx, uint16_t x, uint16_t y, uint16_t width, uint16_t height, COLOR color);
void fill_rectangle_by_corners(COLOR *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color);
void draw_line(COLOR *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color);
void draw_circle(COLOR *buf, uint16_t bx, int x0, int y0, int radius, COLOR color);
void fill_circle(COLOR *buf, uint16_t bx, int x0, int y0, int radius, COLOR color);
void draw_ellipse(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color);
void fill_ellipse(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color);
void bit_blit(COLOR *src, COLOR *dst, uint16_t src_bx, uint16_t dst_bx, uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t dst_x, uint16_t dst_y);

#ifdef __cplusplus
	}
#endif

#endif
