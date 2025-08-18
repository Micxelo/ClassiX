/*
	ui/font.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/font.h>
#include <ClassiX/typedef.h>

#define PSF1_MAGIC							(0x0436)
#define PSF1_MODE512						(0x01)
#define PSF1_MODEHASTAB						(0x02)
#define PSF1_MODEHASSEQ						(0x04)
#define PSF1_MAXMODE						(0x05)
#define PSF1_SEPARATOR						(0xffff)
#define PSF1_STARTSEQ						(0xfffe)

typedef struct __attribute__((packed)) {
	uint16_t magic;
	uint8_t mode;
	uint8_t charsize;
} PSF1_HEADER;

#define PSF2_MAGIC							(0x864ab572)
#define PSF2_HAS_UNICODE_TABLE				(0x01)
#define PSF2_MAXVERSION 					(0)
#define PSF2_SEPARATOR						(0xff)
#define PSF2_STARTSEQ						(0xfe)

typedef struct __attribute__((packed)) {
	uint32_t magic;
	uint32_t version;
	uint32_t headersize;			/* Bitmap 偏移量 */
	uint32_t flags;
	uint32_t length;				/* 字形数 */
	uint32_t charsize;				/* 每个字符所占字节数 */
	uint32_t height, width;			/* 字形最大尺寸 */
	/* charsize = height * ((width + 7) / 8) */
} PSF2_HEADER;

/* 验证 PSF 版本 */
static uint32_t psf_validator(const uint8_t *buf)
{
	const uint16_t magic_low = *(const uint16_t *)buf;
	const uint32_t magic_high = *(const uint32_t *)buf;

	if (magic_low == PSF1_MAGIC) {
		return 1;
	}
	if (magic_high == PSF2_MAGIC) {
		return 2;
	}
	return 0;
}

/*
	@brief 载入内存中的 PSF 字体。
	@param buf 内存字体文件缓冲区
	@return 适用于绘图的 BITMAP_FONT 对象指针。
*/
BITMAP_FONT psf_load(const uint8_t *buf)
{
	BITMAP_FONT font = { };
	if (!buf) {
		return font;
	}

	uint32_t version = psf_validator(buf);
	if (version == 1) {
		const PSF1_HEADER *p1 = (const PSF1_HEADER *)buf;
		font.width = 8;
		font.height = p1->charsize;
		font.charsize = p1->charsize;
		font.buf = (uint8_t *) (buf + sizeof(PSF1_HEADER));
	} else if (version == 2) {
		const PSF2_HEADER *p2 = (const PSF2_HEADER *)buf;
		const uint32_t charsize = p2->charsize;
		font.width = p2->width;
		font.height = p2->height;
		font.charsize = charsize;
		font.buf = (uint8_t *) (buf + p2->headersize);
	}
	return font;
}
