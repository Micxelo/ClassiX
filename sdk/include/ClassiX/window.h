/*
	include/ClassiX/window.h
*/

#ifndef _CLASSIX_WINDOW_H_
#define _CLASSIX_WINDOW_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "palette.h"
#include "typedef.h"

#include <stdint.h>

extern HANDLE cx_window_create(const char* title, uint32_t style, uint16_t width, uint16_t height);
extern void cx_window_draw_rectangle(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height, COLOR color, uint16_t border_width);
extern void cx_window_draw_rectangle_by_corners(HANDLE hwnd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, COLOR color, uint16_t border_width);
extern void cx_window_fill_rectangle(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height, COLOR color);
extern void cx_window_fill_rectangle_by_corners(HANDLE hwnd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, COLOR color);
extern void cx_window_draw_line(HANDLE hwnd, int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, COLOR color);
extern void cx_window_draw_circle(HANDLE hwnd, int16_t center_x, int16_t center_y, int16_t radius, COLOR color);
extern void cx_window_fill_circle(HANDLE hwnd, int16_t center_x, int16_t center_y, int16_t radius, COLOR color);
extern void cx_window_draw_ellipse(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height, COLOR color);
extern void cx_window_fill_ellipse(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height, COLOR color);
extern void cx_window_draw_ascii_string(HANDLE hwnd, int16_t x, int16_t y, COLOR color, const char* string, uint32_t font_id);
extern void cx_window_draw_unicode_string(HANDLE hwnd, int16_t x, int16_t y, COLOR color, const char* string, uint32_t font_id);
extern void cx_window_refresh(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height);

#ifdef __cplusplus
	}
#endif

#endif
