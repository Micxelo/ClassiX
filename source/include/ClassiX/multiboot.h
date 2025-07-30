/*
	include/ClassiX/multiboot.h
*/

/*
	Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
	ANY DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#ifndef MULTIBOOT_HEADER
#define MULTIBOOT_HEADER

#define MULTIBOOT_SEARC						8192			/* 从文件起始处搜索头部的字节数 */
#define MULTIBOOT_HEADER_ALIGN				4

#define MULTIBOOT_HEADER_MAGIC				0x1BADB002		/* magic 字段应包含此值 */
#define MULTIBOOT_BOOTLOADER_MAGIC			0x2BADB002		/* 此值应在 eax 中 */


#define MULTIBOOT_MOD_ALIGN					0x00001000		/* multiboot 模块的对齐方式 */

#define MULTIBOOT_INFO_ALIGN				0x00000004		/* multiboot 信息结构体的对齐方式 */

/* 在 Multiboot Header 'flags' 标志 */
#define MULTIBOOT_PAGE_ALIGN				0x00000001		/* 所有启动模块在 i386 页（4KB）边界对齐 */
#define MULTIBOOT_MEMORY_INFO				0x00000002		/* 向操作系统传递内存信息 */
#define MULTIBOOT_VIDEO_MODE				0x00000004		/* 向操作系统传递视频信息 */
#define MULTIBOOT_AOUT_KLUDGE				0x00010000		/* 此标志表示头部中地址字段的使用 */

/* 在 Multiboot 信息结构体 'flags' 标志 */
#define MULTIBOOT_INFO_MEMORY				0x00000001		/* 是否有基本的低/高端内存信息 */
#define MULTIBOOT_INFO_BOOTDEV				0x00000002		/* 是否设置了启动设备 */
#define MULTIBOOT_INFO_CMDLINE				0x00000004		/* 是否定义了命令行 */
#define MULTIBOOT_INFO_MODS					0x00000008		/* 是否有可用的模块 */
/* 以下两个互斥 */
#define MULTIBOOT_INFO_AOUT_SYMS			0x00000010		/* 是否加载了符号表 */
#define MULTIBOOT_INFO_ELF_SHDR				0X00000020		/* 是否有 ELF 段头表 */
#define MULTIBOOT_INFO_MEM_MAP				0x00000040		/* 是否有完整的内存映射 */
#define MULTIBOOT_INFO_DRIVE_INFO			0x00000080		/* 是否有驱动器信息 */
#define MULTIBOOT_INFO_CONFIG_TABLE			0x00000100		/* 是否有配置表 */
#define MULTIBOOT_INFO_BOOT_LOADER_NAME		0x00000200		/* 是否有引导加载器名称 */
#define MULTIBOOT_INFO_APM_TABLE			0x00000400		/* 是否有 APM 表 */
#define MULTIBOOT_INFO_VBE_INFO				0x00000800		/* 是否有视频信息 */
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO		0x00001000

#ifndef ASM_FILE

typedef unsigned char multiboot_uint8_t;
typedef unsigned short multiboot_uint16_t;
typedef unsigned int multiboot_uint32_t;
typedef unsigned long long multiboot_uint64_t;

struct multiboot_header {
	multiboot_uint32_t magic;		/* 必须为 MULTIBOOT_MAGIC */
	multiboot_uint32_t flags;		/* 功能标志 */
	multiboot_uint32_t checksum;	/* 上述字段加上此字段的和必须等于 0 (mod 2^32) */

	/* 仅当设置了 MULTIBOOT_AOUT_KLUDGE 时这些字段才有效 */
	multiboot_uint32_t header_addr;
	multiboot_uint32_t load_addr;
	multiboot_uint32_t load_end_addr;
	multiboot_uint32_t bss_end_addr;
	multiboot_uint32_t entry_addr;

	/* 仅当设置了 MULTIBOOT_VIDEO_MODE 时这些字段才有效 */
	multiboot_uint32_t mode_type;
	multiboot_uint32_t width;
	multiboot_uint32_t height;
	multiboot_uint32_t depth;
};

/* a.out 的符号表 */
struct multiboot_aout_symbol_table {
	multiboot_uint32_t tabsize;
	multiboot_uint32_t strsize;
	multiboot_uint32_t addr;
	multiboot_uint32_t reserved;
};
typedef struct multiboot_aout_symbol_table multiboot_aout_symbol_table_t;

/* ELF 的段头表 */
struct multiboot_elf_section_header_table {
	multiboot_uint32_t num;
	multiboot_uint32_t size;
	multiboot_uint32_t addr;
	multiboot_uint32_t shndx;
};
typedef struct multiboot_elf_section_header_table multiboot_elf_section_header_table_t;

struct multiboot_info {
	/* Multiboot 信息版本号 */
	multiboot_uint32_t flags;

	/* BIOS 提供的可用内存 */
	multiboot_uint32_t mem_lower;
	multiboot_uint32_t mem_upper;

	/* root 分区 */
	multiboot_uint32_t boot_device;

	/* 内核命令行 */
	multiboot_uint32_t cmdline;

	/* 启动模块列表 */
	multiboot_uint32_t mods_count;
	multiboot_uint32_t mods_addr;

	union {
		multiboot_aout_symbol_table_t aout_sym;
		multiboot_elf_section_header_table_t elf_sec;
	} u;

	/* 内存映射缓冲区 */
	multiboot_uint32_t mmap_length;
	multiboot_uint32_t mmap_addr;

	/* 驱动器信息缓冲区 */
	multiboot_uint32_t drives_length;
	multiboot_uint32_t drives_addr;

	/* ROM 配置表 */
	multiboot_uint32_t config_table;

	/* 引导加载器名称 */
	multiboot_uint32_t boot_loader_name;

	/* APM 表 */
	multiboot_uint32_t apm_table;

	/* 视频 */
	multiboot_uint32_t vbe_control_info;
	multiboot_uint32_t vbe_mode_info;
	multiboot_uint16_t vbe_mode;
	multiboot_uint16_t vbe_interface_seg;
	multiboot_uint16_t vbe_interface_off;
	multiboot_uint16_t vbe_interface_len;

	multiboot_uint64_t framebuffer_addr;
	multiboot_uint32_t framebuffer_pitch;
	multiboot_uint32_t framebuffer_width;
	multiboot_uint32_t framebuffer_height;
	multiboot_uint8_t framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED	0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB		1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT	2
	multiboot_uint8_t framebuffer_type;
	union {
		struct {
			multiboot_uint32_t framebuffer_palette_addr;
			multiboot_uint16_t framebuffer_palette_num_colors;
		};
		struct {
			multiboot_uint8_t framebuffer_red_field_position;
			multiboot_uint8_t framebuffer_red_mask_size;
			multiboot_uint8_t framebuffer_green_field_position;
			multiboot_uint8_t framebuffer_green_mask_size;
			multiboot_uint8_t framebuffer_blue_field_position;
			multiboot_uint8_t framebuffer_blue_mask_size;
		};
	};
};
typedef struct multiboot_info multiboot_info_t;

struct multiboot_color {
	multiboot_uint8_t red;
	multiboot_uint8_t green;
	multiboot_uint8_t blue;
};

struct multiboot_mmap_entry {
	multiboot_uint32_t size;
	multiboot_uint64_t addr;
	multiboot_uint64_t len;
#define MULTIBOOT_MEMORY_AVAILABLE			1
#define MULTIBOOT_MEMORY_RESERVED			2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE	3
#define MULTIBOOT_MEMORY_NVS				4
#define MULTIBOOT_MEMORY_BADRAM				5
	multiboot_uint32_t type;
} __attribute__((packed));
typedef struct multiboot_mmap_entry multiboot_memory_map_t;

struct multiboot_mod_list {
	/* 使用的内存范围为 mod_start 到 mod_end-1（包含） */
	multiboot_uint32_t mod_start;
	multiboot_uint32_t mod_end;

	/* 模块命令行 */
	multiboot_uint32_t cmdline;

	/* 填充到 16 字节（必须为零） */
	multiboot_uint32_t pad;
};
typedef struct multiboot_mod_list multiboot_module_t;

/* APM BIOS 信息 */
struct multiboot_apm_info {
	multiboot_uint16_t version;
	multiboot_uint16_t cseg;
	multiboot_uint32_t offset;
	multiboot_uint16_t cseg_16;
	multiboot_uint16_t dseg;
	multiboot_uint16_t flags;
	multiboot_uint16_t cseg_len;
	multiboot_uint16_t cseg_16_len;
	multiboot_uint16_t dseg_len;
};

#endif /* ! ASM_FILE */

#endif /* ! MULTIBOOT_HEADER */
