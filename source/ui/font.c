/*
	ui/font.c
*/

#include <ClassiX/font.h>
#include <ClassiX/memory.h>
#include <ClassiX/typedef.h>

#include <string.h>

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

/*
	@brief 验证内存中的 PSF 字体文件。
	@param buf 内存字体文件缓冲区
	@return PSF 版本号，0 表示无效字体
*/
static uint32_t psf_validator(const uint8_t *buf)
{
	const uint16_t magic_low = *((const uint16_t *) buf);
	const uint32_t magic_high = *((const uint32_t *) buf);

	if (magic_low == PSF1_MAGIC)
		return 1;
	if (magic_high == PSF2_MAGIC)
		return 2;
	return 0;
}

/*
	@brief 解析 PSF1 Unicode 表。
	@param font 字体对象
	@param table_data Unicode 表数据指针
	@param table_size Unicode 表数据大小
*/
static void parse_psf1_unicode_table(const BITMAP_FONT *font, const uint8_t *table_data, uint32_t table_size)
{
	uint32_t current_char = 0;
	uint32_t i = 0;
	
	while (i < table_size && current_char < font->count) {
		uint8_t byte = table_data[i++];
		
		if (byte == 0xFF) {
			/* 分隔符，移动到下一个字符 */
			current_char++;
		} else {
			/* 解析 UTF-8 序列 */
			uint32_t code_point = 0;
			
			if ((byte & 0x80) == 0) {
				/* 单字节 UTF-8 */
				code_point = byte;
			} else if ((byte & 0xE0) == 0xC0 && i < table_size) {
				/* 双字节 UTF-8 */
				uint8_t byte2 = table_data[i++];
				if ((byte2 & 0xC0) == 0x80)
					code_point = ((byte & 0x1F) << 6) | (byte2 & 0x3F);
			} else if ((byte & 0xF0) == 0xE0 && i + 1 < table_size) {
				/* 三字节 UTF-8 */
				uint8_t byte2 = table_data[i++];
				uint8_t byte3 = table_data[i++];
				if ((byte2 & 0xC0) == 0x80 && (byte3 & 0xC0) == 0x80)
					code_point = ((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
			}
			
			if (code_point > 0 && current_char < font->count)
				font->unicode_map[current_char] = code_point;
		}
	}
}

/*
	@brief 解析 PSF2 Unicode 表。
	@param font 字体对象
	@param table_data Unicode 表数据指针
	@param table_size Unicode 表数据大小
*/
static void parse_psf2_unicode_table(const BITMAP_FONT *font, const uint8_t *table_data, uint32_t table_size)
{
	uint32_t current_char = 0;
	uint32_t i = 0;

	while (i < table_size && current_char < font->count) {
		/* 读取 Unicode 码点序列 */
		while (i < table_size) {
			uint16_t unicode = 0;
			uint8_t byte = table_data[i++];
			
			if (byte == PSF2_SEPARATOR) {
				/* 分隔符，移动到下一个字符 */
				current_char++;
				break;
			} else if (byte == PSF2_STARTSEQ) {
				/* 序列开始标记，跳过 */
				continue;
			} else {
				/* 解析 Unicode 码点*/
				if (byte < 0x80) {
					unicode = byte;
				} else if ((byte & 0xE0) == 0xC0 && i < table_size) {
					uint8_t byte2 = table_data[i++];
					unicode = ((byte & 0x1F) << 6) | (byte2 & 0x3F);
				} else if ((byte & 0xF0) == 0xE0 && i + 1 < table_size) {
					uint8_t byte2 = table_data[i++];
					uint8_t byte3 = table_data[i++];
					unicode = ((byte & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F);
				}
				
				if (unicode > 0 && current_char < font->count)
					font->unicode_map[current_char] = unicode;
			}
		}
	}
}

/*
	@brief 载入内存中的 PSF 字体。
	@param buf 内存字体文件缓冲区
	@param size 字体文件总大小
	@return 适用于绘图的 BITMAP_FONT 对象指针
*/
BITMAP_FONT psf_load(const uint8_t *buf, size_t size)
{
	BITMAP_FONT font = { };
	if (!buf || size == 0) 
		return font;

	uint32_t version = psf_validator(buf);
	if (version == 1) {
		const PSF1_HEADER *p1 = (const PSF1_HEADER *) buf;
		
		/* 检查文件大小是否足够容纳头部 */
		if (size < sizeof(PSF1_HEADER))
			return font;
		
		font.width = 8;
		font.height = p1->charsize;
		font.charsize = p1->charsize;
		font.version = 1;
		
		/* 计算字符数量 */
		font.count = p1->mode & PSF1_MODE512 ? 512 : 256;
		
		/* 计算字符数据大小 */
		uint32_t data_size = font.count * font.charsize;
		
		/* 检查文件大小是否足够容纳字符数据 */
		if (size < sizeof(PSF1_HEADER) + data_size) {
			/* 文件大小不足，重新计算字符数量 */
			font.count = (size - sizeof(PSF1_HEADER)) / font.charsize;
			data_size = font.count * font.charsize;
		}
		
		/* 分配字符数据缓冲区 */
		font.buf = (uint8_t*) kmalloc(data_size);
		if (font.buf)
			memcpy(font.buf, buf + sizeof(PSF1_HEADER), data_size);
		
		/* 检查是否有 Unicode 表 */
		font.has_unicode = (p1->mode & PSF1_MODEHASTAB) != 0;
		
		/* 分配并初始化 Unicode 映射表 */
		font.unicode_map = (uint32_t*) kmalloc(font.count * sizeof(uint32_t));
		if (font.unicode_map) {
			/* 初始化为 0xFFFFFFFF（无效值） */
			for (uint32_t i = 0; i < font.count; i++)
				font.unicode_map[i] = 0xFFFFFFFF;
			
			/* 解析 Unicode 表 */
			if (font.has_unicode) {
				uint32_t unicode_table_offset = sizeof(PSF1_HEADER) + data_size;
				
				/* 检查 Unicode 表是否在文件范围内 */
				if (unicode_table_offset < size) {
					const uint8_t *unicode_table = buf + unicode_table_offset;
					uint32_t table_size = size - unicode_table_offset;
					parse_psf1_unicode_table(&font, unicode_table, table_size);
				}
			}
		}
		
	} else if (version == 2) {
		const PSF2_HEADER *p2 = (const PSF2_HEADER *) buf;
		
		/* 检查文件大小是否足够容纳头部 */
		if (size < sizeof(PSF2_HEADER))
			return font;
		
		font.width = p2->width;
		font.height = p2->height;
		font.charsize = p2->charsize;
		font.count = p2->length;
		font.version = 2;
		font.has_unicode = (p2->flags & PSF2_HAS_UNICODE_TABLE) != 0;
		
		/* 检查头部大小是否合理 */
		if (p2->headersize < sizeof(PSF2_HEADER) || p2->headersize > size)
			return font;
		
		/* 计算字符数据大小 */
		uint32_t data_size = font.count * font.charsize;
		
		/* 检查文件大小是否足够容纳字符数据 */
		if (size < p2->headersize + data_size) {
			/* 文件大小不足，重新计算字符数量 */
			font.count = (size - p2->headersize) / font.charsize;
			data_size = font.count * font.charsize;
		}
		
		/* 分配字符数据缓冲区 */
		font.buf = (uint8_t*) kmalloc(data_size);
		if (font.buf)
			memcpy(font.buf, buf + p2->headersize, data_size);
		
		/* 分配并初始化 Unicode 映射表 */
		font.unicode_map = (uint32_t*) kmalloc(font.count * sizeof(uint32_t));
		if (font.unicode_map) {
			/* 初始化为 0xFFFFFFFF（无效值） */
			for (uint32_t i = 0; i < font.count; i++)
				font.unicode_map[i] = 0xFFFFFFFF;
			
			/* 解析 Unicode 表 */
			if (font.has_unicode) {
				uint32_t unicode_table_offset = p2->headersize + data_size;
				
				/* 检查 Unicode 表是否在文件范围内 */
				if (unicode_table_offset < size) {
					const uint8_t *unicode_table = buf + unicode_table_offset;
					uint32_t table_size = size - unicode_table_offset;
					parse_psf2_unicode_table(&font, unicode_table, table_size);
				}
			}
		}
	}
	return font;
}

/*
	@brief 释放字体资源。
	@param font 要释放的字体对象
*/
void psf_free(BITMAP_FONT *font)
{
	if (font) {
		if (font->buf) {
			kfree(font->buf);
			font->buf = NULL;
		}
		if (font->unicode_map) {
			kfree(font->unicode_map);
			font->unicode_map = NULL;
		}
	}
}


/*
	@brief 查找 Unicode 字符对应的字体索引.
	@param font 字体对象
	@param unicode Unicode 码点
	@return 字符索引，如果找不到返回 0
*/
uint32_t psf_find_char_index(const BITMAP_FONT *font, uint32_t unicode)
{
	if (!font || !font->unicode_map)
		/* 如果没有 Unicode 表，对于扩展 ASCII 可能有问题 */
		return (unicode < font->count) ? unicode : 0;

	/* 在 Unicode 映射表中查找 */
	for (uint32_t i = 0; i < font->count; i++)
		if (font->unicode_map[i] == unicode)
			return i;

	/* 如果找不到，对于 ASCII 字符尝试直接使用码点 */
	if (unicode < 128 && unicode < font->count)
		return unicode;

	/* 尝试查找替代字符 */
	if (unicode == 0xFFFD)
		/* REPLACEMENT CHARACTER */
		return 0; /* 使用默认字符 */

	/* 返回默认字符 */
	return 0;
}
