/*
	include/ClassiX/graphic.h
*/

#ifndef _CLASSIX_GRAPHIC_H_
#define _CLASSIX_GRAPHIC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/font.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

void draw_rectangle(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t thickness, COLOR color);
void draw_rectangle_by_corners(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t thickness, COLOR color);
void fill_rectangle(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, uint16_t width, uint16_t height, COLOR color);
void fill_rectangle_by_corners(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color);
void draw_line(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, COLOR color);
void draw_circle(uint32_t *buf, uint16_t bx, int32_t x0, int32_t y0, int32_t radius, COLOR color);
void fill_circle(uint32_t *buf, uint16_t bx, int32_t x0, int32_t y0, int32_t radius, COLOR color);
void draw_ellipse(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color);
void fill_ellipse(uint32_t *buf, uint16_t bx, uint16_t x0, uint16_t y0, uint16_t a, uint16_t b, COLOR color);
void bit_blit(const uint32_t *src, uint16_t src_bx, uint16_t src_x, uint16_t src_y, uint16_t width, uint16_t height, uint32_t *dst, uint16_t dst_bx, uint16_t dst_x, uint16_t dst_y);
void draw_ascii_string(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, COLOR color, const char *str, const BITMAP_FONT *font);
void draw_unicode_string(uint32_t *buf, uint16_t bx, uint16_t x, uint16_t y, COLOR color, const char *str, const BITMAP_FONT *font);

#ifdef __cplusplus
	}
#endif

#endif
