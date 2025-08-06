/*
	core/main.c
*/

#include <ClassiX/builtin.h>
#include <ClassiX/cga.h>
#include <ClassiX/cpu.h>
#include <ClassiX/debug.h>
#include <ClassiX/fifo.h>
#include <ClassiX/font.h>
#include <ClassiX/framebuf.h>
#include <ClassiX/graphic.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/keyboard.h>
#include <ClassiX/layer.h>
#include <ClassiX/memory.h>
#include <ClassiX/mouse.h>
#include <ClassiX/multiboot.h>
#include <ClassiX/palette.h>
#include <ClassiX/pit.h>
#include <ClassiX/rtc.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>

#include <string.h>

extern uintptr_t kernel_start_phys;						/* 内核起始物理地址 */
extern uintptr_t kernel_end_phys;						/* 内核结束物理地址 */
extern uintptr_t bss_start;								/* BSS 段起始地址 */
extern uintptr_t bss_end;								/* BSS 段结束地址 */

#define KMSG_QUEUE_SIZE						128			/* 内核消息队列大小 */

#define KEYBOARD_DATA0						256			/* 键盘数据起始索引 */
#define MOUSE_DATA0							512			/* 鼠标数据起始索引 */
#define PIT_DATA0							768			/* PIT 数据起始索引 */

static FIFO kmsg_queue = { };							/* 内核消息队列 */
static uint32_t kmsg_queue_buf[KMSG_QUEUE_SIZE] = { };	/* 内核消息队列缓冲区 */
static MOUSE_DATA mouse_data = { };						/* 鼠标数据结构 */

BITMAP_FONT font_terminus_12n;
BITMAP_FONT font_terminus_16n;
BITMAP_FONT font_terminus_16b;

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

	if (mbi->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
		/* 不支持的帧缓冲类型 */
		cga_printf("Unsupported frame buffer type: %d.\n", mbi->framebuffer_type);
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

	uart_init();

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
	debug("\nFrame buffer info:\n  Address: 0x%llx, Width: %d, Height: %d, Pitch: %d, BPP: %d\n\n",
		mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height, mbi->framebuffer_pitch, mbi->framebuffer_bpp);

	/* 初始化中断服务 */
	init_gdt();
	init_idt();
	init_pic();

	/* 初始化键盘、鼠标、PIT */
	fifo_init(&kmsg_queue, KMSG_QUEUE_SIZE, kmsg_queue_buf, NULL);
	init_keyboard(&kmsg_queue, KEYBOARD_DATA0);
	init_mouse(&kmsg_queue, MOUSE_DATA0);
	init_pit(1000); /* 频率为 1000 Hz */

	sti();

	/* 初始化内存管理 */
	uintptr_t mem_start = (uintptr_t) &kernel_end_phys;
	size_t mem_size = (size_t) (mbi->mem_upper * 1024 - (&kernel_end_phys - &kernel_start_phys));
	memory_init((void*) mem_start, mem_size);

	/* 初始化内嵌资源 */
	font_terminus_12n = psf_load(builtin_terminus12n);
	font_terminus_16n = psf_load(builtin_terminus16n);
	font_terminus_16b = psf_load(builtin_terminus16b);

	for(;;) {
		if (fifo_status(&kmsg_queue) == 0) {
			
		} else {
			uint32_t _data = fifo_pop(&kmsg_queue);
			if (KEYBOARD_DATA0 <= _data && _data < MOUSE_DATA0) {
				/* 键盘数据 */
			} else if (MOUSE_DATA0 <= _data && _data < PIT_DATA0) {
				/* 鼠标数据 */
				if (mouse_decoder(&mouse_data, _data - MOUSE_DATA0)) {

				}
			}
		}
		
		timer_process();
		hlt();
	}
}
