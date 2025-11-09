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
	uint32_t count;			/* 字符数量 */
	uint8_t *buf;			/* 字体数据（不含文件头） */
	bool has_unicode;		/* 是否由 Unicode 映射表 */
	uint32_t *unicode_map;	/* Unicode 映射表 */
	uint8_t version;		/* PSF 版本 */
} BITMAP_FONT;

BITMAP_FONT psf_load(const uint8_t *buf, size_t size);
void psf_free(BITMAP_FONT *font);
uint32_t psf_find_char_index(const BITMAP_FONT *font, uint32_t unicode);

#ifdef __cplusplus
	}
#endif

#endif