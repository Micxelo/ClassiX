/*
	core/main.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/blkdev.h>
#include <ClassiX/cga.h>
#include <ClassiX/cpu.h>
#include <ClassiX/debug.h>
#include <ClassiX/fatfs.h>
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
#include <ClassiX/pci.h>
#include <ClassiX/pit.h>
#include <ClassiX/rtc.h>
#include <ClassiX/task.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>
#include <ClassiX/widgets.h>

#include <ctype.h>
#include <string.h>

#define MODIFIER_LCONTROL					(0b00000001)
#define MODIFIER_LSHIFT						(0b00000010)
#define MODIFIER_LALT						(0b00000100)
#define MODIFIER_LMETA						(0b00001000)

#define MODIFIER_RCONTROL					(0b00010000)
#define MODIFIER_RSHIFT						(0b00100000)
#define MODIFIER_RALT						(0b01000000)
#define MODIFIER_RMETA						(0b10000000)

#define MODIFIER_CTRL						(MODIFIER_LCONTROL | MODIFIER_RCONTROL)
#define MODIFIER_SHIFT						(MODIFIER_LSHIFT | MODIFIER_RSHIFT)
#define MODIFIER_ALT						(MODIFIER_LALT | MODIFIER_RALT)
#define MODIFIER_META						(MODIFIER_LMETA | MODIFIER_RMETA)

extern uintptr_t kernel_start_phys;						/* 内核起始物理地址 */
extern uintptr_t kernel_end_phys;						/* 内核结束物理地址 */
extern uintptr_t bss_start_phys;						/* BSS 段起始地址 */
extern uintptr_t bss_end_phys;							/* BSS 段结束地址 */\

const CLASSIX_HEADER *kernel_header = (CLASSIX_HEADER *) &kernel_start_phys;

BITMAP_FONT font_terminus_12n;
BITMAP_FONT font_terminus_16n;
BITMAP_FONT font_terminus_16b;
BITMAP_FONT font_unifont;

#define KMSG_QUEUE_SIZE						(128)		/* 内核消息队列大小 */

#define KEYBOARD_DATA0						(256)		/* 键盘数据起始索引 */
#define MOUSE_DATA0							(512)		/* 鼠标数据起始索引 */

static FIFO kmsg_queue = { };							/* 内核消息队列 */

void main(multiboot_info_t *mbi)
{
	uint32_t kmsg_queue_buf[KMSG_QUEUE_SIZE] = { }; /* 内核消息队列缓冲区 */

	/* 修饰键 */
	/* --7----6-----5------4-----3----2-----1------0-- */
	/* RMeta RAlt RShift RCtrl LMeta LAlt LShift LCtrl */
	uint8_t key_modifiers = 0;
	/* 扩展键盘扫描码的识别阶段 */
	/* 0 - 无； 1 - 已按下； 2 - 等待释放 */
	uint8_t expanded_key_phase = 0;

	MOUSE_DATA mouse_data = { }; 			/* 鼠标数据结构 */

	int32_t cursor_x, cursor_y;				/* 光标位置 */
	int32_t new_cursor_x, new_cursor_y;		/* 光标移动后的位置 */
	bool cursor_updated = false;			/* 光标是否已更新 */

	uart_init();

	debug("ClassiX Kernel Signature: 0x%08x\n", kernel_header->magic);
	debug("Kernel Version: %u.%u.%u\n", kernel_header->major, kernel_header->minor, kernel_header->patch);
	debug("Kernel Entry Point: 0x%x\n", kernel_header->entry);
	debug("Kernel Size: %u bytes\n", kernel_header->size);
	debug("Kernel Phys: 0x%x - 0x%x\n", (uint32_t) &kernel_start_phys, (uint32_t) &kernel_end_phys);

	debug("\nMultiboot Bootloader Information\n\n");

	/* 内存信息 */
	multiboot_memory_map_t *mmap;
	debug("Available memory size: %u KiB\n", mbi->mem_lower + mbi->mem_upper);
	for (mmap = (multiboot_memory_map_t *) mbi->mmap_addr;
			(uintptr_t) mmap < (mbi->mmap_addr + mbi->mmap_length);
			mmap = (multiboot_memory_map_t *) ((uintptr_t) mmap + mmap->size + sizeof(mmap->size)))
		debug("  Size: 0x%x, Base: 0x%016llx, Length: 0x%016llx, Type: %d\n",
			mmap->size, mmap->addr, mmap->len, mmap->type);

	/* 启动设备信息 */
	debug("\nBoot device: %08x\n", mbi->boot_device);
	switch(mbi->boot_device >> 24) {
		case BIOS_DEV_FD0:
			debug("Boot from floppy 0.\n");
			break;
		case BIOS_DEV_FD1:
			debug("Boot from floppy 1.\n");
			break;
		case BIOS_DEV_HD0:
			debug("Boot from harddisk 0.\n");
			break;
		case BIOS_DEV_HD1:
			debug("Boot from harddisk 0.\n");
			break;
		default:
			debug("Unknown boot device.\n");
	}

	/* Video Mode 信息 */
	debug("\nFrame buffer info:\n  Address: 0x%llx, Width: %u, Height: %u, Pitch: %u, BPP: %u\n\n",
		mbi->framebuffer_addr, mbi->framebuffer_width, mbi->framebuffer_height, mbi->framebuffer_pitch, mbi->framebuffer_bpp);

	/* 启动参数 */
	if (mbi->flags & MULTIBOOT_INFO_CMDLINE)
		debug("Boot comand line: %s\n", (char *) mbi->cmdline);

	/* 初始化内存管理 */
	uintptr_t mem_start = (uintptr_t) &kernel_end_phys;
	size_t mem_size = (size_t) (mbi->mem_upper * 1024 - (&kernel_end_phys - &kernel_start_phys));
	memory_init(&g_mp, (void*) mem_start, mem_size);

	/* 初始化中断服务 */
	init_gdt();
	init_idt();
	init_pic();

	/* 初始化键盘、鼠标 */
	fifo_init(&kmsg_queue, KMSG_QUEUE_SIZE, kmsg_queue_buf, NULL);
	init_keyboard(&kmsg_queue, KEYBOARD_DATA0);
	init_mouse(&kmsg_queue, MOUSE_DATA0);

	/* 初始化多任务 */
	TASK *ktask = init_multitasking();
	kmsg_queue.task = ktask;
	task_register(ktask, PRIORITY_HIGH);

	/* 初始化 PIT */
	init_pit(1000); /* 频率为 1000 Hz */
	/* 初始化 PIC */
	out8(PIC0_IMR,  0b11111000); /* 允许 IRQ0、IRQ1 和 IRQ2 */
	out8(PIC1_IMR,  0b11101111); /* 允许 IRQ12 */

	/* 开放中断 */
	sti();

	/* 初始化内建字体 */
	font_terminus_12n = psf_load(ASSET_DATA(fonts, terminus12n_psf), ASSET_SIZE(fonts, terminus12n_psf));
	font_terminus_16n = psf_load(ASSET_DATA(fonts, terminus16n_psf), ASSET_SIZE(fonts, terminus16n_psf));
	font_terminus_16b = psf_load(ASSET_DATA(fonts, terminus16b_psf), ASSET_SIZE(fonts, terminus16b_psf));

	/* 初始化图层管理 */
	init_framebuffer(mbi);
	layer_init((uint32_t *) g_fb.addr, g_fb.width, g_fb.height);
	/* 背景图层 */
	LAYER *layer_back = layer_alloc(g_fb.width, g_fb.height, false);
	fill_rectangle(layer_back->buf, layer_back->width, 0, 0, layer_back->width, layer_back->height, COLOR_MIKU);
	layer_move(layer_back, 0, 0);
	layer_set_z(layer_back, 0);
	/* 光标图层 */
	LAYER *layer_cursor = cursor_init();
	cursor_x = (g_fb.width - layer_cursor->width) / 2;
	cursor_y = (g_fb.height - layer_cursor->height) / 2;
	layer_move(layer_cursor, cursor_x, cursor_y);
	layer_set_z(layer_cursor, 1);


	/* 扫描 PCI 设备 */
	pci_scan_devices();

	/* 注册块设备 */
	register_blkdevs();

	/* 获取启动设备 */
	BLKDEV *boot_device = get_device((mbi->boot_device >> 24) & 0xFF);
	if (boot_device)
		debug("Boot device found: ID=0x%x, Sector size=%d, Total sectors=%d\n",
			boot_device->id, boot_device->sector_size, boot_device->total_sectors);
	else
		debug("Boot device not found.\n");
		
	/* 初始化根文件系统 */
	fatfs_init(g_fs, boot_device, FAT_TYPE_NONE);

	/* 初始化键盘指示灯 */
	uint8_t keychar;						/* 键盘数据 */
	uint8_t key_leds = 0;					/* LED 状态 */
	uint32_t key_cmd_wait = 0xffffffff;		/* 键盘命令等待标志 */
	FIFO key_cmd_queue;						/* 键盘命令 FIFO */
	uint32_t key_cmd_buf[32] = { };			/* 键盘命令缓冲区 */
	fifo_init(&key_cmd_queue, 32, key_cmd_buf, NULL);
	fifo_push(&key_cmd_queue, KEYCMD_LED);
	fifo_push(&key_cmd_queue, 0); /* 初始状态全部关闭 */

	for (;;) {
		if (fifo_status(&key_cmd_queue) > 0 && key_cmd_wait == 0xffffffff) {
			/* 发送键盘数据 */
			key_cmd_wait = fifo_pop(&key_cmd_queue);
			kbc_send_data((uint8_t) key_cmd_wait);
		}
		cli();
		if (fifo_status(&kmsg_queue) == 0) {
			sti();
			if (cursor_updated) {
				/* 光标位置已更新 */
				layer_move(layer_cursor, new_cursor_x, new_cursor_y);
				cursor_updated = false;
			} else {
				task_sleep(ktask);
			}
		} else {
			uint32_t _data = fifo_pop(&kmsg_queue);
			sti();
			if (KEYBOARD_DATA0 <= _data && _data < MOUSE_DATA0) {
				/* 键盘数据 */
				if (_data == KEYBOARD_DATA0 + EXPANDED_KEY_PREFIX) {
					/* 扩展键前缀 */
					expanded_key_phase = 1;
				}

				/* 将键码转换为字符数据 */
				if (_data < KEYBOARD_DATA0 + 0x80)
					/* 普通键 */
					keychar = (key_modifiers & (MODIFIER_SHIFT | MODIFIER_RSHIFT)) ?
						keymap_us_shift[_data - KEYBOARD_DATA0] : keymap_us_default[_data - KEYBOARD_DATA0];
				else
					keychar = 0;

				/* 字母 */
				if (isupper(keychar))
					if (((key_modifiers & (MODIFIER_SHIFT | MODIFIER_RSHIFT)) && (key_leds & KEYBOARD_LED_CAPS_LOCK)) ||
						((!(key_modifiers & (MODIFIER_SHIFT | MODIFIER_RSHIFT))) && (!(key_leds & KEYBOARD_LED_CAPS_LOCK))))
						keychar += ('a' - 'A');

				/* LControl */
				if (_data == KEYBOARD_DATA0 + 0x1d)
					key_modifiers |= MODIFIER_LCONTROL;
				if (_data == KEYBOARD_DATA0 + 0x9d)
					key_modifiers &= ~MODIFIER_LCONTROL;

				/* RControl */
				if (_data == KEYBOARD_DATA0 + 0x9d && expanded_key_phase == 1) {
					key_modifiers |= MODIFIER_RCONTROL;
					expanded_key_phase = 2;
				}
				if (_data == KEYBOARD_DATA0 + 0x9d && expanded_key_phase == 2) {
					key_modifiers &= ~MODIFIER_RCONTROL;
					expanded_key_phase = 0;
				}
					
				/* RShift */
				if (_data == KEYBOARD_DATA0 + 0x36)
					key_modifiers |= MODIFIER_RSHIFT;
				if (_data == KEYBOARD_DATA0 + 0xb6)
					key_modifiers &= ~MODIFIER_RSHIFT;

				/* LShift */
				if (_data == KEYBOARD_DATA0 + 0x2a)
					key_modifiers |= MODIFIER_LSHIFT;
				if (_data == KEYBOARD_DATA0 + 0xaa)
					key_modifiers &= ~MODIFIER_LSHIFT;
					
				/* RShift */
				if (_data == KEYBOARD_DATA0 + 0x36)
					key_modifiers |= MODIFIER_RSHIFT;
				if (_data == KEYBOARD_DATA0 + 0xb6)
					key_modifiers &= ~MODIFIER_RSHIFT;

				/* LAlt */
				if (_data == KEYBOARD_DATA0 + 0x38)
					key_modifiers |= MODIFIER_LALT;
				if (_data == KEYBOARD_DATA0 + 0xb8)
					key_modifiers &= ~MODIFIER_LALT;

				/* RAlt */
				if (_data == KEYBOARD_DATA0 + 0xb8 && expanded_key_phase == 1) {
					key_modifiers |= MODIFIER_RALT;
					expanded_key_phase = 2;
				}
				if (_data == KEYBOARD_DATA0 + 0xb8 && expanded_key_phase == 2) {
					key_modifiers &= ~MODIFIER_RALT;
					expanded_key_phase = 0;
				}

				/* LMeta */
				if (_data == KEYBOARD_DATA0 + 0x5b && expanded_key_phase == 1) {
					key_modifiers |= MODIFIER_LMETA;
					expanded_key_phase = 2;
				}
				if (_data == KEYBOARD_DATA0 + 0xdb && expanded_key_phase == 2) {
					key_modifiers &= ~MODIFIER_LMETA;
					expanded_key_phase = 0;
				}

				/* RMeta */
				if (_data == KEYBOARD_DATA0 + 0x5c && expanded_key_phase == 1) {
					key_modifiers |= MODIFIER_RMETA;
					expanded_key_phase = 2;
				}
				if (_data == KEYBOARD_DATA0 + 0xdc && expanded_key_phase == 2) {
					key_modifiers &= ~MODIFIER_RMETA;
					expanded_key_phase = 0;
				}
				
				/* Menu */
				if (_data == KEYBOARD_DATA0 + 0x5d && expanded_key_phase == 1) {
					expanded_key_phase = 2;
				}
				if (_data == KEYBOARD_DATA0 + 0xdd && expanded_key_phase == 2) {
					expanded_key_phase = 0;
				}

				/* Caps Lock */
				if (_data == KEYBOARD_DATA0 + 0x3a) {
					key_leds ^= KEYBOARD_LED_CAPS_LOCK;
					fifo_push(&key_cmd_queue, KEYCMD_LED);
					fifo_push(&key_cmd_queue, key_leds);
				}

				/* Num Lock */
				if (_data == KEYBOARD_DATA0 + 0x45) {
					key_leds ^= KEYBOARD_LED_NUM_LOCK;
					fifo_push(&key_cmd_queue, KEYCMD_LED);
					fifo_push(&key_cmd_queue, key_leds);
				}

				/* Scroll Lock */
				if (_data == KEYBOARD_DATA0 + 0x46) {
					key_leds ^= KEYBOARD_LED_SCROLL_LOCK;
					fifo_push(&key_cmd_queue, KEYCMD_LED);
					fifo_push(&key_cmd_queue, key_leds);
				}

				/* 键盘成功接收数据 */
				if (_data == KEYBOARD_DATA0 + 0xfa)
					key_cmd_wait = 0xffffffff;
				
				/* 键盘未能接收数据 */
				if (_data == KEYBOARD_DATA0 + 0xfe)
					/* 重新发送上次的数据 */
					if (key_cmd_wait != 0xffffffff)
						kbc_send_data((uint8_t) key_cmd_wait);
			} else {
				/* 鼠标数据 */
				if (mouse_decoder(&mouse_data, _data - MOUSE_DATA0)) {
					/* 移动光标 */
					cursor_x += mouse_data.dx;
					cursor_y += mouse_data.dy;
					/* 修正超出屏幕的值 */
					if (cursor_x < 0) cursor_x = 0;
					if (cursor_y < 0) cursor_y = 0;
					if (cursor_x > (signed) (g_fb.width - 1)) cursor_x = g_fb.width - 1;
					if (cursor_y > (signed) (g_fb.height - 1)) cursor_y = g_fb.height - 1;
					new_cursor_x = cursor_x;
					new_cursor_y = cursor_y;
					cursor_updated = true;

					if (mouse_data.button) {
						/* 按下鼠标 */
						if (mouse_data.button & MOUSE_LBUTTON) {
							/* 按下鼠标左键 */
						} else if (mouse_data.button & MOUSE_RBUTTON) {
							/* 按下鼠标右键 */
						} else if (mouse_data.button & MOUSE_MBUTTON) {
							/* 按下鼠标中键 */
						}
					} else {
						/* 松开鼠标 */

					}
				}
			}
		}

		hlt();
	}
}
