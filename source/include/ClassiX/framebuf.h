/*
	include/ClassiX/framebuf.h
*/

#ifndef _CLASSIX_FRAMEBUF_H_
#define _CLASSIX_FRAMEBUF_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/multiboot.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

typedef struct {
	uintptr_t addr;					/* 帧缓冲地址 */
	uint32_t pitch;					/* 帧缓冲行距 */
	uint32_t width;					/* 帧缓冲宽度 */
	uint32_t height;				/* 帧缓冲高度 */
	uint8_t bpp;					/* 帧缓冲位深度 */
	uint8_t red_field_position;		/* R 字段位置 */
	uint8_t red_mask_size;			/* R 掩码大小 */
	uint8_t green_field_position;	/* G 字段位置 */
	uint8_t green_mask_size;		/* G 掩码大小 */
	uint8_t blue_field_position;	/* B 字段位置 */
	uint8_t blue_mask_size;			/* B 掩码大小 */
} FRAMEBUFFER;

extern FRAMEBUFFER g_fb;			/* 全局帧缓冲区 */

int32_t init_framebuffer(multiboot_info_t *mbi);
COLOR get_pixel(uint16_t x, uint16_t y);
void set_pixel(uint16_t x, uint16_t y, COLOR color);

#ifdef __cplusplus
	}
#endif

#endif