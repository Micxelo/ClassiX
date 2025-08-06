/*
	ui/framebuf.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/framebuf.h>
#include <ClassiX/multiboot.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

FRAMEBUFFER g_fb;

int init_framebuffer(multiboot_info_t *mbi)
{
	if ((!mbi && mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
		debug("Failed to initialize global framebuffer.\n");
		return -1; /* 未提供帧缓冲信息 */
	}

	g_fb.addr = mbi->framebuffer_addr;
	g_fb.pitch = mbi->framebuffer_pitch;
	g_fb.width = mbi->framebuffer_width;
	g_fb.height = mbi->framebuffer_height;
	g_fb.bpp = mbi->framebuffer_bpp;

	g_fb.red_field_position = mbi->framebuffer_red_field_position;
	g_fb.red_mask_size = mbi->framebuffer_red_mask_size;
	g_fb.green_field_position = mbi->framebuffer_green_field_position;
	g_fb.green_mask_size = mbi->framebuffer_green_mask_size;
	g_fb.blue_field_position = mbi->framebuffer_blue_field_position;
	g_fb.blue_mask_size = mbi->framebuffer_blue_mask_size;

	debug("Global framebuffer initialized.\n");
	return 0; /* 成功初始化帧缓冲 */
}

/* 将颜色分量扩展到 8 位精度 */
static inline uint8_t expand_to_8bit(uint32_t value, uint8_t src_bits)
{
	if (src_bits >= 8)
		return (value >> (src_bits - 8)) & 0xff;

	/* 线性扩展：value * 255 / (2^src_bits - 1) */
	if (src_bits > 0) {
		uint32_t max_val = (1 << src_bits) - 1;
		return (value * 255 + max_val / 2) / max_val;
	}

	return 0;
}

COLOR get_pixel(uint16_t x, uint16_t y)
{
	if (x >= g_fb.width || y >= g_fb.height)
		return COLOR32(0); /* 超出帧缓冲范围，返回黑色 */

	uintptr_t pixel_address = g_fb.addr + y * g_fb.pitch + x * (g_fb.bpp / 8);

	/* 根据位深度读取原始像素值 */
	uint32_t raw_pixel = 0;
	switch (g_fb.bpp) {
		case 8:
			raw_pixel = *((uint8_t *) pixel_address);
			break;
		case 15: /* 实际是 15 位（5-5-5），但通常用 16 位存储 */
		case 16:
			raw_pixel = *((uint16_t *) pixel_address);
			break;
		case 24: /* 24位像素需要特殊处理（3字节） */
			raw_pixel = *((uint8_t *) pixel_address) |
						 (*((uint8_t *) pixel_address + 1) << 8) |
						 (*((uint8_t *) pixel_address + 2) << 16);
			break;
		case 32:
			raw_pixel = *((uint32_t *)pixel_address);
			break;
		default: /* 不支持的位深度 */
			return COLOR32(0);
	}

	/* 从原始像素值提取 RGB 分量 */
	uint32_t r = (raw_pixel >> g_fb.red_field_position) & ((1 << g_fb.red_mask_size) - 1);
	uint32_t g = (raw_pixel >> g_fb.green_field_position) & ((1 << g_fb.green_mask_size) - 1);
	uint32_t b = (raw_pixel >> g_fb.blue_field_position) & ((1 << g_fb.blue_mask_size) - 1);

	/* 将分量扩展到 8 位精度 */
	r = expand_to_8bit(r, g_fb.red_mask_size);
	g = expand_to_8bit(g, g_fb.green_mask_size);
	b = expand_to_8bit(b, g_fb.blue_mask_size);

	return COLOR32_FROM_RGBA(r, g, b, 0xff);
}

void set_pixel(uint32_t x, uint32_t y, COLOR color)
{
	if (x >= g_fb.width || y >= g_fb.height)
		return; /* 超出帧缓冲范围 */

	/* 提取 RGB 颜色分量 */
	uint8_t r = color.r;
	uint8_t g = color.g;
	uint8_t b = color.b;

	/* 计算像素地址 */
	uintptr_t pixel_address = g_fb.addr + y * g_fb.pitch;

	/* 根据位深度处理不同格式 */
	switch (g_fb.bpp) {
		case 15:
		case 16: {
			/* 计算各分量值 */
			uint32_t r_val = (r >> (8 - g_fb.red_mask_size)) << g_fb.red_field_position;
			uint32_t g_val = (g >> (8 - g_fb.green_mask_size)) << g_fb.green_field_position;
			uint32_t b_val = (b >> (8 - g_fb.blue_mask_size)) << g_fb.blue_field_position;

			/* 组合为16位像素值 */
			uint16_t pixel = (uint16_t) (r_val | g_val | b_val);
			*((uint16_t *) (pixel_address + x * 2)) = pixel;
		}
		break;

		case 24: {
			uint8_t *pixel_ptr = (uint8_t *) (pixel_address + x * 3);

			/* 根据字段位置确定字节顺序 */
			if (g_fb.blue_field_position == 0) {
				/* BGR 顺序 */
				pixel_ptr[0] = b;
				pixel_ptr[1] = g;
				pixel_ptr[2] = r;
			} else {
				/* RGB 顺序 */
				pixel_ptr[0] = r;
				pixel_ptr[1] = g;
				pixel_ptr[2] = b;
			}
		}
		break;

		case 32: {
			/* 使用颜色掩码构建像素值 */
			uint32_t pixel = 0;

			/* 红色分量 */
			if (g_fb.red_mask_size > 0) {
				uint32_t r_val = (r >> (8 - g_fb.red_mask_size)) << g_fb.red_field_position;
				pixel |= r_val;
			}

			/* 绿色分量 */
			if (g_fb.green_mask_size > 0) {
				uint32_t g_val = (g >> (8 - g_fb.green_mask_size)) << g_fb.green_field_position;
				pixel |= g_val;
			}

			/* 蓝色分量 */
			if (g_fb.blue_mask_size > 0) {
				uint32_t b_val = (b >> (8 - g_fb.blue_mask_size)) << g_fb.blue_field_position;
				pixel |= b_val;
			}

			/* 写入帧缓冲区 */
			*((uint32_t *) (pixel_address + x * 4)) = pixel;
		}
		break;
	}
}


