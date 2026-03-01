/*
	core/main.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/blkdev.h>
#include <ClassiX/buzzer.h>
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
#include <ClassiX/window.h>

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

#define DBLCLK_THRESHOLD_MS					(500)		/* 双击最大时间间隔（毫秒） */
#define DBLCLK_THRESHOLD_DIST 				(4)			/* 双击最大移动距离（像素）) */

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

static void init_fpu(void);
static bool is_single_click(int32_t x, int32_t y, BTN_CLICK_STATE *state);
static bool is_double_click(uint32_t now, int32_t x, int32_t y, BTN_CLICK_STATE *state);
static inline void update_click_state(uint32_t now, int32_t x, int32_t y, BTN_CLICK_STATE *state);

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
	MOUSE_STATE_MANAGER ms_mgr = { };		/* 鼠标状态 */

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

	/* 初始化 FPU */
	init_fpu();

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

	int32_t drag_start_x = 0, drag_start_y = 0;					/* 拖动起始位置 */
	int32_t drag_window_start_x = 0, drag_window_start_y = 0;	/* 拖动窗口起始位置 */
	int32_t new_window_x = 0, new_window_y = 0;					/* 窗口移动后的位置 */
	bool dragging = false;										/* 是否正在拖动窗口 */
	bool window_updated = false;								/* 窗口位置是否已更新 */
	extern LAYER *layer_focused;								/* 获得焦点的窗口图层 */
	LAYER *layer_dragged = NULL;								/* 正在拖动的图层 */
	LAYER *layer_captured = NULL;								/* 捕获鼠标的图层 */

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
	demo_terminal();

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
				if (_data == KEYBOARD_DATA0 + EXPANDED_KEY_PREFIX) {
					/* 扩展键前缀 */
					expanded_key_phase = 1;
					continue;
				}

				bool is_extended = (expanded_key_phase != 0);				/* 是否为扩展键 */
				uint8_t raw = _data - KEYBOARD_DATA0;						/* 原始扫描码 */
				uint8_t scancode = raw & 0x7F;								/* 扫描码 */
				uint32_t keycode = scancode | (is_extended ? 0x80 : 0);		/* 键码 */

				bool is_down = is_extended ? (expanded_key_phase == 1) : ((raw & 0x80) == 0);

				/* 发送 KEYBOARD_KEYDOWN / KEYBOARD_KEYUP 事件 */
				if (layer_focused && layer_focused->window && layer_focused->window->task) {
					EVENT event = {
						.window = layer_focused->window,
						.id = is_down ? EVENT_KEYBOARD_KEYDOWN : EVENT_KEYBOARD_KEYUP,
						.param = keycode
					};
					fifo_push_event(&layer_focused->window->task->fifo, &event);
				}

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

				/* 发送 KEYPRESS 事件 */
				if (keychar && layer_focused && layer_focused->window && layer_focused->window->task) {
					EVENT event = {
						.window = layer_focused->window,
						.id = EVENT_KEYBOARD_KEYPRESS,
						.param = keychar
					};
					fifo_push_event(&layer_focused->window->task->fifo, &event);
				}

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
					/* 记录当前时间 */
					uint64_t current_time = get_system_milliseconds();

					/* 移动光标 */
					cursor_x += mouse_data.dx;
					cursor_y += mouse_data.dy;

					/* 修正超出屏幕的值 */
					if (cursor_x < - (int) cursor_current->anchor_x)
						cursor_x = - (int) cursor_current->anchor_x;
					if (cursor_x > (signed) (g_fb.width - 1) - (int) cursor_current->anchor_x)
						cursor_x = (signed) (g_fb.width - 1) - (int) cursor_current->anchor_x;
					if (cursor_y < - (int) cursor_current->anchor_y)
						cursor_y = - (int) cursor_current->anchor_y;
					if (cursor_y > (signed) (g_fb.height - 1) - (int) cursor_current->anchor_y)
						cursor_y = (signed) (g_fb.height - 1) - (int) cursor_current->anchor_y;

					/* 更新坐标 */
					new_cursor_x = cursor_x;
					new_cursor_y = cursor_y;
					cursor_updated = true;

					LAYER *layer_target = NULL; /* 光标下方的图层 */
					int32_t rx = 0, ry = 0; /* 光标相对于图层的坐标 */

					if (dragging && layer_dragged) {
						/* 系统正在拖拽窗口 */
						layer_target = layer_dragged;
					} else if (layer_captured) {
						/* 窗口捕获鼠标 */
						/* 即使光标移出窗口，事件依然发给该窗口 */
						layer_target = layer_captured;
					} else {
						/* 从顶层向下查找窗口 */
						for (int32_t z = g_lm.top - 1; z > 0; z--) {
							LAYER *current_layer = g_lm.layers[z];
							if (!current_layer->window) continue;

							int32_t _rx = cursor_x - current_layer->x;
							int32_t _ry = cursor_y - current_layer->y;

							if (0 <= _rx && _rx < current_layer->width && 0 <= _ry && _ry < current_layer->height) {
								/* 检查透明度，确保不是点在透明像素上 */
								if (GET_PIXEL32(current_layer->buf, current_layer->width, _rx, _ry).a) {
									layer_target = current_layer;
									rx = _rx;
									ry = _ry;
									break; /* 找到最上层的窗口 */
								}
							}
						}
					}

					/* 相对于目标窗口的坐标 */
					if (layer_target) {
						rx = cursor_x - layer_target->x;
						ry = cursor_y - layer_target->y;
					}

					/* 确定目标窗口 */
					WINDOW *win_target = (layer_target && layer_target->window) ? layer_target->window : NULL;
					HIT_RESULT hit = HIT_NONE;

					/* 处理滚轮事件 */
					if (mouse_data.dz != 0 && win_target && win_target->task && !dragging) {
						/* 只发送给当前焦点窗口，或者光标下的窗口 */
						EVENT event = {
							.window = win_target,
							.id = EVENT_MOUSE_WHEEL,
							.param = mouse_data.dz
						};
						fifo_push_event(&win_target->task->fifo, &event);
					}

					/* 计算按键变化 */
					int32_t changed_buttons = mouse_data.button ^ ms_mgr.last_buttons;
					int32_t buttons_down = mouse_data.button & changed_buttons; /* 本次循环按下的键 */

					/* 如果有任意键按下，且没有在系统拖拽/捕获中，尝试激活窗口 */
					if (buttons_down && win_target && !dragging && !layer_captured) {
						window_activate(win_target);
						/* 进行命中测试 */
						hit = window_hit_test(win_target, rx, ry);
					} else if (win_target) {
						/* 更新命中状态 */
						if (layer_captured)
							hit = HIT_CLIENT;
						else
							hit = window_hit_test(win_target, rx, ry);
					}

					/* 存在目标窗口、鼠标位于客户区、非拖拽状态，发送 EVENT_MOUSE_MOVE 事件 */
					if (win_target && win_target->task && !dragging && hit == HIT_CLIENT) {
						EVENT event = {
							.window = win_target,
							.id = EVENT_MOUSE_MOVE,
							/* 转换为客户区坐标 */
							.point.x = rx - win_target->client_x,
							.point.y = ry - win_target->client_y
						};
						fifo_push_event(&win_target->task->fifo, &event);
					}

					/* 鼠标左键 */
					if (changed_buttons & MOUSE_LBUTTON) {
						int16_t client_x = win_target ? (rx - win_target->client_x) : 0;
						int16_t client_y = win_target ? (ry - win_target->client_y) : 0;

						if (mouse_data.button & MOUSE_LBUTTON) {
							/* 按下左键 */
							if (!dragging && win_target) {
								if (hit == HIT_CLIENT) {
									/* 击中客户区 */
									/* 发送按下事件 */
									EVENT event = {
										.window = win_target,
										.id = EVENT_MOUSE_LBTNDOWN,
										.point.x = client_x,
										.point.y = client_y
									};
									fifo_push_event(&win_target->task->fifo, &event);

									/* 检测并发送双击事件 */
									if (is_double_click(current_time, cursor_x, cursor_y, &ms_mgr.left)) {
										EVENT dbl_event = {
											.window = win_target,
											.id = EVENT_MOUSE_LDBCLK,
											.point.x = client_x,
											.point.y = client_y
										};
										fifo_push_event(&win_target->task->fifo, &dbl_event);
										ms_mgr.left.double_clicked = true; /* 标记双击已发生 */
									} else {
										ms_mgr.left.double_clicked = false;
									}

									/* 更新点击状态记录 */
									update_click_state(current_time, cursor_x, cursor_y, &ms_mgr.left);

									/* 开启捕获 */
									layer_captured = layer_target;
								} else {
									dragging = true;
									if (hit == HIT_CLOSE_BUTTON) {
										/* 击中关闭按钮 */
										/* 发送关闭事件 */
										EVENT event = {
											.window = win_target,
											.id = EVENT_WINDOW_CLOSING,
											.param = CLOSING_BY_CLOSE_BUTTON
										};
										fifo_push_event(&win_target->task->fifo, &event);
									} else if (hit == HIT_TITLEBAR) {
										/* 击中标题栏 */
										/* 移动窗口 */
										drag_start_x = cursor_x;
										drag_start_y = cursor_y;
										drag_window_start_x = layer_target->x;
										drag_window_start_y = layer_target->y;
										new_window_x = layer_target->x;
										new_window_y = layer_target->y;
										layer_dragged = layer_target;
										window_updated = true;
									}
								}
							}
						} else {
							/* 释放左键 */
							if (!dragging && win_target) {
								/* 发送松开事件 */
								EVENT event = {
									.window = win_target,
									.id = EVENT_MOUSE_LBTNUP,
									.point.x = client_x,
									.point.y = client_y
								};
								fifo_push_event(&win_target->task->fifo, &event);

								/* 发送单击事件 */
								if (!ms_mgr.left.double_clicked) {
									if (is_single_click(cursor_x, cursor_y, &ms_mgr.left)) {
										EVENT click_event = {
											.window = win_target,
											.id = EVENT_MOUSE_LCLICK,
											.point.x = client_x,
											.point.y = client_y
										};
										fifo_push_event(&win_target->task->fifo, &click_event);
									}
								} else {
									ms_mgr.left.double_clicked = false;
								}
							}

							/* 结束移动窗口 */
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

					/* 鼠标右键 */
					if (changed_buttons & MOUSE_RBUTTON) {
						int16_t client_x = win_target ? (rx - win_target->client_x) : 0;
						int16_t client_y = win_target ? (ry - win_target->client_y) : 0;

						if (mouse_data.button & MOUSE_RBUTTON) {
							/* 按下右键 */
							if (!dragging && win_target) {
								if (hit == HIT_CLIENT) {
									/* 击中客户区 */
									/* 发送按下事件 */
									EVENT event = {
										.window = win_target,
										.id = EVENT_MOUSE_RBTNDOWN,
										.point.x = client_x,
										.point.y = client_y
									};
									fifo_push_event(&win_target->task->fifo, &event);

									/* 检测并发送双击事件 */
									if (is_double_click(current_time, cursor_x, cursor_y, &ms_mgr.right)) {
										EVENT dbl_event = {
											.window = win_target,
											.id = EVENT_MOUSE_RDBCLK,
											.point.x = client_x,
											.point.y = client_y
										};
										fifo_push_event(&win_target->task->fifo, &dbl_event);
										ms_mgr.right.double_clicked = true; /* 标记双击已发生 */
									} else {
										ms_mgr.right.double_clicked = false;
									}

									/* 更新点击状态记录 */
									update_click_state(current_time, cursor_x, cursor_y, &ms_mgr.right);

									/* 开启捕获 */
									layer_captured = layer_target;
								}
							}
						} else {
							/* 释放右键 */
							if (!dragging && win_target) {
								/* 发送松开事件 */
								EVENT event = {
									.window = win_target,
									.id = EVENT_MOUSE_RBTNUP,
									.point.x = client_x,
									.point.y = client_y
								};
								fifo_push_event(&win_target->task->fifo, &event);

								/* 发送单击事件 */
								if (!ms_mgr.right.double_clicked) {
									if (is_single_click(cursor_x, cursor_y, &ms_mgr.right)) {
										EVENT click_event = {
											.window = win_target,
											.id = EVENT_MOUSE_RCLICK,
											.point.x = client_x,
											.point.y = client_y
										};
										fifo_push_event(&win_target->task->fifo, &click_event);
									}
								} else {
									ms_mgr.right.double_clicked = false;
								}
							}
						}
					}

					/* 鼠标中键 */
					if (changed_buttons & MOUSE_MBUTTON) {
						int16_t client_x = win_target ? (rx - win_target->client_x) : 0;
						int16_t client_y = win_target ? (ry - win_target->client_y) : 0;

						if (mouse_data.button & MOUSE_MBUTTON) {
							/* 按下中键 */
							if (!dragging && win_target) {
								if (hit == HIT_CLIENT) {
									/* 击中客户区 */
									/* 发送按下事件 */
									EVENT event = {
										.window = win_target,
										.id = EVENT_MOUSE_MBTNDOWN,
										.point.x = client_x,
										.point.y = client_y
									};
									fifo_push_event(&win_target->task->fifo, &event);

									/* 检测并发送双击事件 */
									if (is_double_click(current_time, cursor_x, cursor_y, &ms_mgr.middle)) {
										EVENT dbl_event = {
											.window = win_target,
											.id = EVENT_MOUSE_MDBCLK,
											.point.x = client_x,
											.point.y = client_y
										};
										fifo_push_event(&win_target->task->fifo, &dbl_event);
										ms_mgr.middle.double_clicked = true; /* 标记双击已发生 */
									} else {
										ms_mgr.middle.double_clicked = false;
									}

									/* 更新点击状态记录 */
									update_click_state(current_time, cursor_x, cursor_y, &ms_mgr.middle);

									/* 开启捕获 */
									layer_captured = layer_target;
								}
							}
						} else {
							/* 释放中键 */
							if (!dragging && win_target) {
								/* 发送松开事件 */
								EVENT event = {
									.window = win_target,
									.id = EVENT_MOUSE_MBTNUP,
									.point.x = client_x,
									.point.y = client_y
								};
								fifo_push_event(&win_target->task->fifo, &event);

								/* 发送单击事件 */
								if (!ms_mgr.middle.double_clicked) {
									if (is_single_click(cursor_x, cursor_y, &ms_mgr.middle)) {
										EVENT click_event = {
											.window = win_target,
											.id = EVENT_MOUSE_MCLICK,
											.point.x = client_x,
											.point.y = client_y
										};
										fifo_push_event(&win_target->task->fifo, &click_event);
									}
								} else {
									ms_mgr.middle.double_clicked = false;
								}
							}
						}
					}

					/* 如果所有按键都已松开，取消捕获 */
					if (mouse_data.button == 0)
						layer_captured = NULL;

					/* 窗口位置更新 */
					if (dragging && layer_dragged) {
						new_window_x = drag_window_start_x + (cursor_x - drag_start_x);
						new_window_y = drag_window_start_y + (cursor_y - drag_start_y);
						window_updated = true; /* 标记需要刷新 */
					}

					/* 记录按键状态 */
					ms_mgr.last_buttons = mouse_data.button;
				}
			}
		}
	}
}

/*
	初始化 FPU。
*/
static void init_fpu(void)
{
	CR0 cr0 = { .value = load_cr0() };
	CR4 cr4 = { .value = load_cr4() };

	/* 启用 x87 FPU */
	cr0.value = load_cr0();
	cr0.em = 0; /* 允许 x87 FPU 指令 */
	cr0.mp = 1; /* 任务切换时触发 NM 异常，实现惰性保存 */
	cr0.ne = 1; /* 启用 FPU 异常处理 */
	store_cr0(cr0.value);

	/* 开启 SSE */
	cr4.osfxsr = 1; /* 允许使用 fxsave/fxrstor 以及无掩码的 SSE 异常 */
	cr4.osxmmexcpt = 1; /* 操作系统支持未屏蔽的 SIMD 浮点异常 */
	store_cr4(cr4.value);

	asm volatile ("fninit"); /* 初始化 x87 FPU 状态 */
}

/*
	@brief 判断是否为单击事件。
	@param x 当前 X 坐标（绝对坐标）
	@param y 当前 Y 坐标（绝对坐标）
	@param state 上一次点击的状态
	@return 如果满足单击条件，返回 true；否则返回 false。
*/
static bool is_single_click(int32_t x, int32_t y, BTN_CLICK_STATE *state)
{
	int32_t dx = x - state->last_down_x;
	int32_t dy = y - state->last_down_y;

	if (dx < 0) dx = -dx;
	if (dy < 0) dy = -dy;

	if (dx <= DBLCLK_THRESHOLD_DIST && dy <= DBLCLK_THRESHOLD_DIST)
		return true;

	return false;
}

/*
	@brief 判断是否为双击事件。
	@param now 当前时间（毫秒）
	@param x 当前 X 坐标（绝对坐标）
	@param y 当前 Y 坐标（绝对坐标）
	@param state 上一次点击的状态
	@return 如果满足双击条件，返回 true；否则返回 false。
*/
static bool is_double_click(uint32_t now, int32_t x, int32_t y, BTN_CLICK_STATE *state)
{
	if (now - state->last_down_time < DBLCLK_THRESHOLD_MS)
		if (is_single_click(x, y, state))
			return true;
	return false;
}

/*
	@brief 更新点击状态。
	@param now 当前时间（毫秒）
	@param x 当前 X 坐标（绝对坐标）
	@param y 当前 Y 坐标（绝对坐标）
	@param state 点击状态结构体
*/
static inline void update_click_state(uint32_t now, int32_t x, int32_t y, BTN_CLICK_STATE *state)
{
	state->last_down_time = now;
	state->last_down_x = x;
	state->last_down_y = y;
}
