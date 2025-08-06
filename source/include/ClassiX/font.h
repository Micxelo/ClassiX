/*
	include/ClassiX/font.h
*/

#ifndef _CLASSIX_FONT_H_
#define _CLASSIX_FONT_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef struct {
	uint32_t width, height;
	uint32_t charsize;		/* 单个字符所占字节数 */
	uint8_t *buf;			/* 字体数据（不含文件头） */
} BITMAP_FONT;

BITMAP_FONT psf_load(const uint8_t *buf);

#ifdef __cplusplus
	}
#endif

#endif