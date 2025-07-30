/*
	core/main.c
*/

#include <ClassiX/typedef.h>
#include <ClassiX/multiboot.h>
#include <ClassiX/io.h>
#include <ClassiX/graphic.h>
#include <ClassiX/debug.h>
#include <ClassiX/rtc.h>
#include <ClassiX/int.h>

void main(uint32_t mb_magic, multiboot_info_t *mbi)
{
	cga_clear();

	if (mb_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		/* 非 Multiboot 启动 */
		cga_printf("Invalid magic number: 0x%x\n", mb_magic);
		return;
	}

	init_gdtidt();
	init_pic();
	uart_init();

	sti();

	debug("\nMultiboot Bootloader Information\n\n");

	if (mbi->flags & MULTIBOOT_INFO_MEMORY) {
		/* 传递内存信息 */
		multiboot_memory_map_t *mmap;
		debug("Available memory size: %d KiB\n", mbi->mem_lower + mbi->mem_upper);
		for (mmap = (multiboot_memory_map_t *) mbi->mmap_addr;
			 (uintptr_t) mmap < (mbi->mmap_addr + mbi->mmap_length);
			 mmap = (multiboot_memory_map_t *) ((uintptr_t) mmap + mmap->size + sizeof(mmap->size))) {
			debug("  Size:0x%x, Base:0x%016llx, Length:0x%016llx, Type:%d\n",
				mmap->size, mmap->addr, mmap->len, mmap->type);
		}
	}

	if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
		/* 传递 Video Mode 信息 */
		debug("\nFrame buffer info:\n  Type: %s, Address: 0x%llx, Width: %d, Height: %d\n",
			"INDEXED\0RGB\0    EGATEXT\0" + 8 * mbi->framebuffer_type,
			mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);
	}

	for(;;) {

	}
}
