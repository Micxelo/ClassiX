/*
	core/main.c
*/

#include <ClassiX/cpu.h>
#include <ClassiX/debug.h>
#include <ClassiX/graphic.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/keyboard.h>
#include <ClassiX/multiboot.h>
#include <ClassiX/rtc.h>
#include <ClassiX/typedef.h>

void main(uint32_t mb_magic, multiboot_info_t *mbi)
{
	cga_clear();

	if (mb_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		/* 非 Multiboot 启动 */
		cga_printf("Invalid magic number: 0x%x .\n", mb_magic);
		return;
	}

	if (!(mbi->flags & MULTIBOOT_INFO_MEMORY)) {
		/* 未传递内存信息 */
		cga_printf("Memory information is unavailable.");
		return;
	}

	if (!(mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
		/* 未传递视频信息 */
		cga_printf("Frame buffer information is unavailable.");
		return;
	}

	init_gdt();
	init_idt();
	init_pic();
	uart_init();
	
	sti();

	debug("\nMultiboot Bootloader Information\n\n");

	/* 内存信息 */
	multiboot_memory_map_t *mmap;
	debug("Available memory size: %d KiB\n", mbi->mem_lower + mbi->mem_upper);
	for (mmap = (multiboot_memory_map_t *) mbi->mmap_addr;
			(uintptr_t) mmap < (mbi->mmap_addr + mbi->mmap_length);
			mmap = (multiboot_memory_map_t *) ((uintptr_t) mmap + mmap->size + sizeof(mmap->size))) {
		debug("  Size:0x%x, Base:0x%016llx, Length:0x%016llx, Type:%d\n",
			mmap->size, mmap->addr, mmap->len, mmap->type);
	}

	/* 传递 Video Mode 信息 */
	debug("\nFrame buffer info:\n  Type: %s, Address: 0x%llx, Width: %d, Height: %d\n\n",
		"INDEXED\0RGB\0    EGATEXT\0" + 8 * mbi->framebuffer_type,
		mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);

	for(;;) {

	}
}
