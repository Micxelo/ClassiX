/*
	core/main.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/blkdev.h>
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
extern uintptr_t bss_end_phys;							/* BSS 段结束地址 */

const CLASSIX_HEADER *kernel_header = (CLASSIX_HEADER *) &kernel_start_phys;

BITMAP_FONT font_terminus_12n;
BITMAP_FONT font_terminus_16n;
BITMAP_FONT font_terminus_16b;
BITMAP_FONT font_unifont;

#define KMSG_QUEUE_SIZE						(128)		/* 内核消息队列大小 */

static FIFO kmsg = { };									/* 内核消息队列 */

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
	bool cursor_updated = false;			/* 光标位置是否已更新 */

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
	fifo_init(&kmsg, KMSG_QUEUE_SIZE, kmsg_queue_buf, NULL);
	init_keyboard(&kmsg, KEYBOARD_DATA0);
	init_mouse(&kmsg, MOUSE_DATA0);

	/* 初始化多任务 */
	TASK *ktask = init_multitasking();
	kmsg.task = ktask;
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
	new_cursor_x = cursor_x;
	new_cursor_y = cursor_y;
	layer_move(layer_cursor, cursor_x, cursor_y);
	layer_set_z(layer_cursor, 1);
	/* 获得焦点的窗口图层 */
	LAYER *layer_focused = NULL;
	
	int32_t drag_start_x = 0, drag_start_y = 0;					/* 拖动起始位置 */
	int32_t drag_window_start_x = 0, drag_window_start_y = 0;	/* 拖动窗口起始位置 */
	int32_t new_window_x = 0, new_window_y = 0;					/* 窗口移动后的位置 */
	bool dragging = false;										/* 是否正在拖动窗口 */
	bool window_updated = false;								/* 窗口位置是否已更新 */
	LAYER *layer_dragged = NULL;								/* 正在拖动的图层 */

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

	/* 运行演示程序 */
	extern TASK *demo_terminal(void);
	TASK *terminal_task = demo_terminal();

	for (;;) {
		if (fifo_status(&key_cmd_queue) > 0 && key_cmd_wait == 0xffffffff) {
			/* 发送键盘数据 */
			key_cmd_wait = fifo_pop(&key_cmd_queue);
			kbc_send_data((uint8_t) key_cmd_wait);
		}
		cli();
		if (fifo_status(&kmsg) == 0) {
			sti();
			if (cursor_updated) {
				/* 光标位置已更新 */
				layer_move(layer_cursor, new_cursor_x, new_cursor_y);
				cursor_updated = false;
			} else if (window_updated && layer_dragged) {
				/* 窗口位置已更新 */
				layer_move(layer_dragged, new_window_x, new_window_y);
			} else {
				task_sleep(ktask);
			}
		} else {
			uint32_t _data = fifo_pop(&kmsg);
			sti();
			if (KEYBOARD_DATA0 <= _data && _data < MOUSE_DATA0) {
				/* 键盘数据 */
				if (_data == KEYBOARD_DATA0 + EXPANDED_KEY_PREFIX)
					/* 扩展键前缀 */
					expanded_key_phase = 1;

				/* 将键码转换为字符数据 */
				if (_data < KEYBOARD_DATA0 + 0x80)
					/* 普通键 */
					keychar = (key_modifiers & (MODIFIER_SHIFT | MODIFIER_RSHIFT)) ?
						keymap_us_shift[_data - KEYBOARD_DATA0] : keymap_us_default[_data - KEYBOARD_DATA0];
				else
					keychar = 0;

				/* 字母 */
				if (isalpha(keychar)) {
					bool shift_pressed = (key_modifiers & (MODIFIER_SHIFT | MODIFIER_RSHIFT)) != 0;
					bool caps_lock_on = (key_leds & KEYBOARD_LED_CAPS_LOCK) != 0;
					
					if (shift_pressed != caps_lock_on) {
						if (islower(keychar))
							keychar = toupper(keychar);
					} else {
						if (isupper(keychar))
							keychar = tolower(keychar);
					}
				}

				/* 小键盘区处理 */
				if (_data >= KEYBOARD_DATA0 + 0x47 && _data <= KEYBOARD_DATA0 + 0x53)
					/* 小键盘区键码范围 */
					if (!(key_leds & KEYBOARD_LED_NUM_LOCK))
						if ('0' <= keychar && keychar <= '9')
							keychar = 0;

				if (keychar && g_lm.top > 1)
					/* 有效字符，发送到获得焦点的窗口 */
					fifo_push(&terminal_task->fifo, keychar + KEYBOARD_DATA0);

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
							if (!dragging) {
								/* 寻找最上方的窗口 */
								for (int32_t z = g_lm.top - 1; z >= 0; z--) {
									LAYER *current_layer = g_lm.layers[z];
									if (!current_layer->window)
										continue;
									
									int32_t rx = cursor_x - current_layer->x;	/* 相对窗口的 X 坐标 */
									int32_t ry = cursor_y - current_layer->y;	/* 相对窗口的 Y 坐标 */

									if (0 <= rx && rx < current_layer->width && 0 <= ry && ry < current_layer->height) {
										if (GET_PIXEL32(current_layer->buf, current_layer->width, rx, ry).a) {
											/* 非透明区域 */
											layer_set_z(current_layer, g_lm.top - 1);

											if (WD_WINDOW_IS_HOLDING_TITLEBAR(current_layer->window, rx, ry)) {
												/* 进入窗口移动模式 */
												drag_start_x = cursor_x;
												drag_start_y = cursor_y;
												drag_window_start_x = current_layer->x;
												drag_window_start_y = current_layer->y;
												new_window_x = current_layer->x;
												new_window_y = current_layer->y;
												layer_dragged = current_layer;
												dragging = true;
												window_updated = true;
												break;
											}
										}
									}
								}
							} else {
								/* 正在拖动窗口 */
								if (layer_dragged) {
									/* 新位置 = 窗口初始位置 + (当前鼠标位置 - 拖动起始位置) */
									new_window_x = drag_window_start_x + (cursor_x - drag_start_x);
									new_window_y = drag_window_start_y + (cursor_y - drag_start_y);
								}
							}
						} else if (mouse_data.button & MOUSE_RBUTTON) {
							/* 按下鼠标右键 */
						} else if (mouse_data.button & MOUSE_MBUTTON) {
							/* 按下鼠标中键 */
						}
					} else {
						/* 松开鼠标 */
						if (dragging) {
							dragging = false;
							if (layer_dragged && window_updated) {
								layer_move(layer_dragged, new_window_x, new_window_y);
								window_updated = false;
								layer_dragged = NULL;
							}
						}
					}
				}
			}
		}

		hlt();
	}
}
