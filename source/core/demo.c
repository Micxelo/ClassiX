/*
	core/demo.c
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
#include <ClassiX/io.h>
#include <ClassiX/keyboard.h>
#include <ClassiX/layer.h>
#include <ClassiX/memory.h>
#include <ClassiX/mouse.h>
#include <ClassiX/palette.h>
#include <ClassiX/pci.h>
#include <ClassiX/pit.h>
#include <ClassiX/rtc.h>
#include <ClassiX/task.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>
#include <ClassiX/widgets.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	int32_t cursor_x;		/* 光标 X 坐标 */
	int32_t cursor_y;		/* 光标 Y 坐标 */
	char prompt[20];		/* 提示符 */
	char cmdline[256];		/* 命令行缓冲区 */
	int cmdline_pos;		/* 命令行缓冲区位置 */
	WINDOW hWnd;			/* 窗口句柄 */
} TERMINAL;

void terminal_putchar(TERMINAL *terminal, char c, bool move_cursor);
void terminal_newline(TERMINAL *terminal);
void terminal_puts(TERMINAL *terminal, const char *s);
int32_t terminal_printf(TERMINAL *terminal, const char *format, ...);

/* 绘制字符串并刷新 */
static void layer_draw_string(LAYER *layer, int32_t x, int32_t y, COLOR fc, COLOR bc, const char *s, const BITMAP_FONT *font)
{
	size_t len = strlen(s);

	fill_rectangle(layer->buf, layer->width, x, y, len * font->width, font->height, bc);
	draw_unicode_string(layer->buf, layer->width, x, y, fc, s, font);
	layer_refresh(layer, x, y, x + len * font->width, y + font->height);
}

/* help 命令 */
static void terminal_cmd_help(TERMINAL *terminal)
{
	terminal_printf(terminal, "Available commands:\n");
	terminal_printf(terminal, "  cat      - Display file content\n");
	terminal_printf(terminal, "  clear    - Clear screen\n");
	terminal_printf(terminal, "  echo     - Echo arguments\n");
	terminal_printf(terminal, "  help     - Show this help\n");
	terminal_printf(terminal, "  ls       - List directory contents\n");
	terminal_printf(terminal, "  sysinfo  - Display system information\n");
	terminal_printf(terminal, "  time     - Show current time\n");
}

/* clear 命令 */
static void terminal_cmd_clear(TERMINAL *terminal)
{
	fill_rectangle(
		terminal->hWnd.layer->buf,
		terminal->hWnd.layer->width,
		1,
		19,
		terminal->hWnd.layer->width - 2,
		terminal->hWnd.layer->height - 20,
		COLOR_CGA_BLACK);
	layer_refresh(terminal->hWnd.layer, 1, 19, terminal->hWnd.layer->width - 2, terminal->hWnd.layer->height - 2);
	terminal->cursor_x = 1;
	terminal->cursor_y = 19;
}

/* time 命令 */
static void terminal_cmd_time(TERMINAL *terminal)
{
	const char *day_names[] = {
		"Sunday", "Monday", "Tuesday", "Wednesday",
		"Thursday", "Friday", "Saturday"};

	terminal_printf(terminal,
		"%04d/%02d/%02d %02d:%02d:%02d  %s\n",
		get_year(), get_month(), get_day_of_month(),
		get_hour(), get_minute(), get_second(),
		day_names[get_day_of_week()]);
}

/* echo 命令 */
static void terminal_cmd_echo(TERMINAL *terminal)
{
	const char *args = terminal->cmdline + 5;
	while (*args == ' ')
		args++; /* 跳过多余空格 */
	terminal_printf(terminal, "%s\n", args);
}

/* cat 命令 */
static void terminal_cmd_cat(TERMINAL *terminal)
{
	const char *filepath = terminal->cmdline + 4;

	/* 跳过路径前的空格 */
	while (*filepath == ' ')
		filepath++;

	if (*filepath == '\0') {
		terminal_printf(terminal, "Usage: cat <filepath>\n");
		return;
	}

	/* 打开文件 */
	FAT_FILE file;
	int32_t result = fatfs_open_file(&file, g_fs, filepath);

	if (result == FATFS_SUCCESS) {
		/* 文件打开成功，读取并显示内容 */
		char buffer[512];
		uint32_t bytes_read;
		uint32_t total_read = 0;
		bool error = false;

		do {
			result = fatfs_read_file(&file, buffer, sizeof(buffer) - 1, &bytes_read);

			if (result == FATFS_SUCCESS && bytes_read > 0) {
				/* 确保缓冲区以空字符结尾 */
				buffer[bytes_read] = '\0';

				/* 显示文件内容 */
				terminal_puts(terminal, buffer);
				total_read += bytes_read;
			} else if (result != FATFS_SUCCESS) {
				terminal_printf(terminal, "\nRead error: %d\n", result);
				error = true;
				break;
			}
		} while (bytes_read == sizeof(buffer) - 1);

		if (!error)
			terminal_printf(terminal, "\nFile size: %u bytes\n", total_read);

		/* 关闭文件 */
		fatfs_close_file(&file);
	}
	else {
		/* 文件打开失败 */
		terminal_printf(terminal, "Failed to open file: %s (error: %d)\n", filepath, result);
	}
}

/* ls 命令 */
static void terminal_cmd_ls(TERMINAL *terminal)
{
	const char *dirpath = "/"; /* 默认根目录 */

	if (strncmp(terminal->cmdline, "ls ", 3) == 0) {
		dirpath = terminal->cmdline + 3;
		/* 跳过路径前的空格 */
		while (*dirpath == ' ')
			dirpath++;
		if (*dirpath == '\0')
			dirpath = "/";
	}

	/* 获取目录项 */
	FAT_DIRENTRY entries[32];
	int32_t entries_read;

	int32_t result = fatfs_get_directory_entries(g_fs, dirpath, entries, sizeof(entries) / sizeof(entries[0]), &entries_read);

	if (result == FATFS_SUCCESS) {
		terminal_printf(terminal, "Directory: %s\n", dirpath);

		int dir_count = 0, file_count = 0;

		for (int i = 0; i < entries_read; i++) {
			FAT_DIRENTRY *entry = &entries[i];

			/* 跳过空目录项和卷标 */
			if (entry->filename[0] == 0x00 || entry->filename[0] == 0xE5 || (entry->attributes & FAT_ATTR_VOLUME_ID))
				continue;

			/* 显示文件名 */
			char filename[13];
			fatfs_convert_from_83((char *) entry->filename, (char *) entry->extension, filename);

			/* 显示文件属性 */
			char attr_str[32];
			fatfs_get_attribute_names(entry->attributes, attr_str, sizeof(attr_str));

			if (entry->attributes & FAT_ATTR_DIRECTORY) {
				/* 目录 */
				terminal_printf(terminal, "%-12s %14c %s\n", filename, ' ', attr_str);
				dir_count++;
			} else {
				/* 文件 */
				terminal_printf(terminal, "%-12s %8u bytes %s\n", filename, entry->file_size, attr_str);
				file_count++;
			}
		}

		terminal_printf(terminal, "Total: %d entries (%d files, %d directories)\n", entries_read, file_count, dir_count);
	} else {
		terminal_printf(terminal, "Failed to list directory: %s (error: %d)\n", dirpath, result);
	}
}

/* sysinfo 命令 */
static void terminal_cmd_sysinfo(TERMINAL *terminal)
{
	/* CPU 信息 */
	terminal_printf(terminal, "CPU Information:\n");

	if (check_cpuid_support()) {
		terminal_printf(terminal, "  CPUID: Supported\n");

		/* 获取 CPU 厂商 */
		char vendor[13];
		get_cpu_vendor(vendor);
		terminal_printf(terminal, "  Vendor: %s\n", vendor);

		/* 获取 CPU 品牌 */
		char brand[49];
		get_cpu_brand(brand);
		terminal_printf(terminal, "  Brand: %s\n", brand);

		/* 获取 APIC ID */
		uint32_t apic_id = get_apic_id();
		terminal_printf(terminal, "  APIC ID: %u\n", apic_id);

		/* 检查 TSC 支持 */
		if (check_tsc_support()) {
			terminal_printf(terminal, "  TSC: Supported");

			/* 检查 TSC 不变性 */
			if (check_tsc_invariant())
				terminal_printf(terminal, "  Invariant TSC: Yes\n");
			else
				terminal_printf(terminal, "  Invariant TSC: No\n");
		} else {
			terminal_printf(terminal, "  TSC: Not Supported\n");
		}

		/* 获取缓存信息 */
		int32_t cache_count = get_cache_count();
		terminal_printf(terminal, "  Cache Count: %d\n", cache_count);

		if (cache_count > 0) {
			terminal_printf(terminal, "  Cache Details:\n");
			for (int i = 0; i < cache_count; i++) {
				cache_info_t cache_info;
				if (get_cache_info(i, &cache_info) == 0) {
					const char *cache_type = "";
					switch (cache_info.level) {
						case 1:
							cache_type = "L1";
							break;
						case 2:
							cache_type = "L2";
							break;
						case 3:
							cache_type = "L3";
							break;
						default:
							cache_type = "Unknown";
							break;
					}

					/* 显示缓存大小 */
					if (cache_info.size >= 1024 * 1024) {
						terminal_printf(terminal, "    %s Cache: %u MiB, Line: %u B\n",
							cache_type, cache_info.size / (1024 * 1024),
							cache_info.line_size);
					} else {
						terminal_printf(terminal, "    %s Cache: %u KiB, Line: %u B\n",
							cache_type, cache_info.size / 1024,
							cache_info.line_size);
					}
				}
			}
		}
	} else {
		terminal_printf(terminal, "  CPUID: Not Supported\n");
	}

	terminal_printf(terminal, "\n");

	/* 内存信息 */
	terminal_printf(terminal, "Memory Information:\n");

	size_t free_memory = get_free_memory(&g_mp);

	size_t total_memory = g_mp.size;
	size_t used_memory = total_memory - free_memory;

	/* 计算使用率 */
	int usage_percent = (used_memory * 100) / total_memory;

	terminal_printf(terminal, "  Total Memory: %u MiB\n", total_memory / (1024 * 1024));
	terminal_printf(terminal, "  Used Memory: %u MiB\n", used_memory / (1024 * 1024));
	terminal_printf(terminal, "  Free Memory: %u MiB\n", free_memory / (1024 * 1024));
	terminal_printf(terminal, "  Usage: %d%%\n", usage_percent);
}

/* run 命令 */
static void terminal_cmd_run(TERMINAL *terminal)
{
	FAT_FILE file;
	fatfs_open_file(&file, g_fs, "/system/hello.srv");
	extern int32_t app_start(FAT_FILE *file, const char *cmdline);
	app_start(&file, NULL);
}

/* unknown 命令 */
static void terminal_cmd_unknown(TERMINAL *terminal)
{
	terminal_printf(terminal, "Unknown command: %s\n", terminal->cmdline);
	terminal_printf(terminal, "Type 'help' for available commands.\n");
}

/* 处理命令行 */
static void terminal_process_cmdline(TERMINAL *terminal)
{
	terminal_printf(terminal, "\n");

	if (strcmp(terminal->cmdline, "help") == 0)
		terminal_cmd_help(terminal);
	else if (strcmp(terminal->cmdline, "clear") == 0)
		terminal_cmd_clear(terminal);
	else if (strcmp(terminal->cmdline, "time") == 0)
		terminal_cmd_time(terminal);
	else if (strncmp(terminal->cmdline, "echo ", 5) == 0)
		terminal_cmd_echo(terminal);
	else if (strncmp(terminal->cmdline, "cat ", 4) == 0)
		terminal_cmd_cat(terminal);
	else if (strcmp(terminal->cmdline, "ls") == 0 || strncmp(terminal->cmdline, "ls ", 3) == 0)
		terminal_cmd_ls(terminal);
	else if (strcmp(terminal->cmdline, "sysinfo") == 0)
		terminal_cmd_sysinfo(terminal);
	else if (strcmp(terminal->cmdline, "run") == 0)
		terminal_cmd_run(terminal);
	else if (terminal->cmdline[0] != '\0')
		terminal_cmd_unknown(terminal);
		
	terminal_printf(terminal, "\n");
}

void __attribute__((noreturn)) task_terminal_entry(void)
{
	TASK *task = task_get_current();
	TERMINAL terminal;

	window_create(
		&terminal.hWnd,
		48,
		48,
		482,
		320,
		WINSTYLE_SYSCONTROL | WINSTYLE_MOVEABLE,
		"Terminal",
		task
	);
	fill_rectangle(
		terminal.hWnd.layer->buf,
		terminal.hWnd.layer->width,
		1,
		19,
		terminal.hWnd.layer->width - 2,
		terminal.hWnd.layer->height - 20,
		COLOR_CGA_BLACK
	);
	layer_refresh(terminal.hWnd.layer, 0, 0, terminal.hWnd.layer->width, terminal.hWnd.layer->height);

	terminal.cursor_x = 1;
	terminal.cursor_y = 19;
	terminal.cmdline[0] = '\0';
	terminal.cmdline_pos = 0;
	strcpy(terminal.prompt, "ClassiX> ");

	/* 显示提示符 */
	terminal_puts(&terminal, terminal.prompt);

	for (;;) {
		cli();
		if (fifo_status(&task->fifo) == 0) {
			sti();
			task_sleep(task);
		} else {
			uint32_t _data = fifo_pop(&task->fifo);
			sti();
			if (KEYBOARD_DATA0 <= _data && _data < MOUSE_DATA0) {
				/* 键盘数据 */
				char key = _data - KEYBOARD_DATA0;
				
				if (key == 0x08) {
					/* 退格键 */
					if (terminal.cmdline_pos > 0) {
						terminal.cmdline_pos--;
						terminal.cmdline[terminal.cmdline_pos] = '\0';
						terminal_puts(&terminal, "\b \b");
					}
				} else if (key == 0x0d) {
					/* 回车键 */
					/* 处理命令行 */
					terminal_process_cmdline(&terminal);
					
					/* 重置命令行缓冲区 */
					terminal.cmdline[0] = '\0';
					terminal.cmdline_pos = 0;
					
					/* 显示新提示符 */
					terminal_puts(&terminal, terminal.prompt);
				} else if (key == 0x09) {	/* 制表符 */
					/* 可以添加Tab补全功能 */
					terminal_putchar(&terminal, '\t', 1);
				} else {
					/* 普通字符 */
					if (terminal.cursor_x < 477 && terminal.cmdline_pos < 255) {
						/* 添加到命令行缓冲区 */
						terminal.cmdline[terminal.cmdline_pos] = key;
						terminal.cmdline_pos++;
						terminal.cmdline[terminal.cmdline_pos] = '\0';
						
						/* 显示字符 */
						terminal_putchar(&terminal, key, 1);
					}
				}
			}
		}
	}
}

TASK *demo_terminal(void)
{
	TASK *task_terminal = task_alloc();
	task_terminal->tss.esp = (uint32_t) kmalloc(DEFAULT_USER_STACK) + DEFAULT_USER_STACK;
	task_terminal->tss.eip = (uint32_t) &task_terminal_entry;
	task_terminal->tss.es = 0x10;
	task_terminal->tss.cs = 0x08;
	task_terminal->tss.ss = 0x10;
	task_terminal->tss.ds = 0x10;
	task_terminal->tss.fs = 0x10;
	task_terminal->tss.gs = 0x10;
	task_register(task_terminal, PRIORITY_NORMAL);
	fifo_init(&task_terminal->fifo, 128, kmalloc(128 * sizeof(uint32_t)), task_terminal);
	return task_terminal;
}

void terminal_putchar(TERMINAL *terminal, char c, bool move_cursor)
{
	char s[2] = { c, 0 };
	
	switch (c) {
		case 0x08:	/* 退格 */
			if (terminal->cursor_x > (1 + (signed int) strlen(terminal->prompt) * 6))
				terminal->cursor_x -= 6;
			break;
		
		case 0x09:	/* 制表符 */
			for (;;) {
				layer_draw_string(
					terminal->hWnd.layer,
					terminal->cursor_x,
					terminal->cursor_y,
					COLOR_CGA_LIGHTGRAY,
					COLOR_CGA_BLACK,
					" ",
					&font_terminus_12n
				);
				terminal->cursor_x += 6;
				if (terminal->cursor_x == 1 + 6 * 80)
					terminal_newline(terminal);
				if ((terminal->cursor_x - 1) % (6 * 4) == 0)
					break;
			}
			break;

		case 0x0a:	/* 换行 */
			terminal_newline(terminal);
			break;

		case 0x0d: 	/* 回车 */
			break;

		default:	/* 其他字符 */
			layer_draw_string(
				terminal->hWnd.layer,
				terminal->cursor_x,
				terminal->cursor_y,
				COLOR_CGA_LIGHTGRAY,
				COLOR_CGA_BLACK,
				s,
				&font_terminus_12n
			);
			if (move_cursor) {
				terminal->cursor_x += 6;
				if (terminal->cursor_x == 1 + 6 * 80)
					terminal_newline(terminal);
			}
			break;
	}
}

void terminal_newline(TERMINAL *terminal)
{
	/* 计算当前行号 (0-based) */
	int current_line = (terminal->cursor_y - 19) / 12;
	
	if (current_line >= 24) {
		/* 滚动一行 */
		int src_y = 19 + 12;
		int dst_y = 19;
		int height = 24 * 12;
		
		/* 复制像素数据 */
		for (int y = 0; y < height; y++) {
			for (int x = 1; x < 1 + 80 * 6; x++) {
				COLOR pixel = GET_PIXEL32(
					terminal->hWnd.layer->buf,
					terminal->hWnd.layer->width,
					x,
					src_y + y
				);
				SET_PIXEL32(
					terminal->hWnd.layer->buf,
					terminal->hWnd.layer->width,
					x,
					dst_y + y,
					pixel
				);
			}
		}
		
		/* 清除最后一行 */
		fill_rectangle(
			terminal->hWnd.layer->buf,
			terminal->hWnd.layer->width,
			1,
			19 + 24 * 12,
			480,
			12,
			COLOR_CGA_BLACK
		);
		
		/* 刷新整个显示区域 */
		layer_refresh(
			terminal->hWnd.layer,
			1,
			19,
			480,
			319
		);
	} else {
		terminal->cursor_y += 12;
	}
	
	/* 无论是否滚动，光标都移动到行首 */
	terminal->cursor_x = 1;
}

void terminal_puts(TERMINAL *terminal, const char *str)
{
	while (*str)
		terminal_putchar(terminal, *str++, 1);
}

int32_t terminal_printf(TERMINAL *terminal, const char *format, ...)
{
	const size_t max_length = 1024;
	char buf[max_length];

	int32_t result;
	va_list va;
	va_start(va, format);

	result = vsnprintf(buf, max_length, format, va);
	va_end(va);

	terminal_puts(terminal, buf);
	return result;
}