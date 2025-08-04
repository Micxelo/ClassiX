/*
	core/main.c
*/

#include <ClassiX/cpu.h>
#include <ClassiX/debug.h>
#include <ClassiX/graphic.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/keyboard.h>
#include <ClassiX/mouse.h>
#include <ClassiX/multiboot.h>
#include <ClassiX/pit.h>
#include <ClassiX/rtc.h>
#include <ClassiX/typedef.h>

#include <string.h>

extern uintptr_t kernel_start_phys;
extern uintptr_t kernel_end_phys;
extern uintptr_t bss_start;
extern uintptr_t bss_end;

#define FIFO_SIZE							128
#define KEYBOARD_DATA0						256
#define MOUSE_DATA0							512
#define PIT_DATA0							768
static FIFO fifo;
static uint32_t fifo_buf[FIFO_SIZE];
static MOUSE_DATA mouse_data = { };

/*
	@brief 检查 Multiboot 启动信息。
	@param mb_magic Multiboot 魔数
	@param mbi Multiboot 信息结构体指针
	@return 如果检查通过，返回 true；否则返回 false。
*/
static bool check_boot_info(uint32_t mb_magic, multiboot_info_t *mbi)
{
	cga_clear();

	if (mb_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		/* 非 Multiboot 启动 */
		cga_printf("Invalid magic number: 0x%x .\n", mb_magic);
		return false;
	}

	if (!(mbi->flags & MULTIBOOT_INFO_MEMORY)) {
		/* 未传递内存信息 */
		cga_printf("Memory information is unavailable.");
		return false;
	}

	if (!(mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)) {
		/* 未传递视频信息 */
		cga_printf("Frame buffer information is unavailable.");
		return false;
	}

	return true;
}

void main(uint32_t mb_magic, multiboot_info_t *mbi)
{

	/* 检查 Multiboot 启动信息 */
	if (!check_boot_info(mb_magic, mbi)) return;

	/* 清零 BSS */
	memset((void*) &bss_start, 0, (size_t) (&bss_end - &bss_start));

	init_gdt();
	init_idt();
	init_pic();
	uart_init();
	fifo_init(&fifo, FIFO_SIZE, fifo_buf, NULL);
	init_keyboard(&fifo, KEYBOARD_DATA0);
	init_mouse(&fifo, MOUSE_DATA0);
	init_pit(1000); /* 初始化 PIT，频率为 1000 Hz */

	debug("\nKernel PHYS: 0x%x - 0x%x\n", (uint32_t) &kernel_start_phys, (uint32_t) &kernel_end_phys);
	debug("\nMultiboot Bootloader Information\n\n");
	/* 内存信息 */
	multiboot_memory_map_t *mmap;
	debug("Available memory size: %d KiB\n", mbi->mem_lower + mbi->mem_upper);
	for (mmap = (multiboot_memory_map_t *) mbi->mmap_addr;
			(uintptr_t) mmap < (mbi->mmap_addr + mbi->mmap_length);
			mmap = (multiboot_memory_map_t *) ((uintptr_t) mmap + mmap->size + sizeof(mmap->size))) {
		debug("  Size: 0x%x, Base: 0x%016llx, Length: 0x%016llx, Type: %d\n",
			mmap->size, mmap->addr, mmap->len, mmap->type);
	}
	/* Video Mode 信息 */
	debug("\nFrame buffer info:\n  Type: %s, Address: 0x%llx, Width: %d, Height: %d\n\n",
		"INDEXED\0RGB\0    EGATEXT\0" + 8 * mbi->framebuffer_type,
		mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height);

	sti();
	

	for(;;) {
		if (fifo_status(&fifo) == 0) {
		} else {
			uint32_t _data = fifo_pop(&fifo);
			if (KEYBOARD_DATA0 <= _data && _data < MOUSE_DATA0) {
				/* 键盘数据 */
			} else if (MOUSE_DATA0 <= _data && _data < PIT_DATA0) {
				/* 鼠标数据 */
				if (mouse_decoder(&mouse_data, _data - MOUSE_DATA0)) {

				}
			}
		}
	}
}
