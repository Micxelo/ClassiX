/*
	core/programs/syscall.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/handle.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/programs.h>
#include <ClassiX/task.h>
#include <ClassiX/font.h>
#include <ClassiX/window.h>
#include <ClassiX/typedef.h>

#define LOW8(u16) 							(uint8_t) ((u16) & 0x00ff)
#define HIGH8(u16) 							(uint8_t) (((u16) & 0xff00) >> 8)
#define LOW16(u32) 							(uint16_t) ((u32) & 0x0000ffff)
#define HIGH16(u32) 						(uint16_t) (((u32) & 0xffff0000) >> 16)

typedef struct {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} registers_t;

/*
	@brief 验证用户空间指针是否合法。
	@param task 当前任务指针
	@param ptr 用户空间指针
	@param size 需要访问的内存大小
	@return true 如果指针合法且可访问返回 true，否则返回 false
*/
static bool verify_user_pointer(TASK *task, uint32_t ptr, uint32_t size)
{
	if (ptr < task->data_base || ptr + size > task->data_base + task->data_limit + 1) {
		debug("SYSCALL: Invalid user pointer: 0x%08x (size: %u)\n", ptr, size);
		return false;
	}
	return true;
}

/*
	@brief 验证用户空间字符串是否合法。
	@param task 当前任务指针
	@param ptr 用户空间字符串偏移
	@return true 如果字符串在用户空间范围内并以 `NULL` 结尾返回 true，否则返回 false
*/
static bool verify_user_string(TASK *task, uint32_t ptr)
{
	if (ptr > task->data_limit) {
		debug("SYSCALL: Invalid user string: 0x%08x\n", task->data_base + ptr);
		return false;
	}

	for (uint32_t offset = ptr; offset <= task->data_limit; offset++) {
		if (*((char *) (task->data_base + offset)) == '\0')
			return true;
	}

	debug("SYSCALL: Unterminated user string: 0x%08x\n", task->data_base + ptr);
	return false;
}

/*
	@brief 根据用户句柄查找窗口对象。
	@param task 当前任务指针
	@param handle_value 窗口句柄值
	@param operation 当前系统调用名
	@return 窗口对象指针，失败返回 NULL
*/
static WINDOW *lookup_user_window(TASK *task, uint32_t handle_value, const char *operation)
{
	HANDLE handle = { .value = handle_value };
	WINDOW *window = (WINDOW *) handle_table_lookup(&task->hwnd_table, handle);

	if (!window || window->signature != WINDOW_STRUCT_SIGNATURE) {
		debug("SYSCALL: Invalid window handle passed to %s: 0x%08x\n", operation, handle_value);
		return NULL;
	}

	return window;
}

/*
	@brief 选择内置字体。
	@param font_id 字体编号
	@return 字体对象指针
*/
static const BITMAP_FONT *select_user_font(uint32_t font_id)
{
	switch (font_id) {
		case WINDOW_FONT_TERMINUS_12N:
			return &font_terminus_12n;
		case WINDOW_FONT_TERMINUS_16N:
			return &font_terminus_16n;
		case WINDOW_FONT_TERMINUS_16B:
			return &font_terminus_16b;
		default:
			return &font_terminus_16n;
	}
}

typedef uint32_t (*syscall_handler_t)(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax);

/*
	@brief 系统调用：退出程序。
	@param eax 系统调用号（应为 `SYSCALL_EXIT_PROCESS`）
	@param ebx 退出代码
	@return 栈顶地址，供 `program_end` 结束程序。
*/
static uint32_t syscall_exit_process(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	uint32_t exit_code = ebx;

	debug("SYSCALL: Program %d exited with code %d.\n", TID(task), exit_code);
	return (uint32_t) &(task->tss.esp0);
}

/*
	@brief 系统调用：调试输出。
	@param eax 系统调用号（应为 `SYSCALL_DEBUG_PRINT`）
	@param ebx 字符串
*/
static uint32_t syscall_debug_print(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	char *str = (char *) (ebx + task->data_base);
	debug("SYSCALL: Debug print from program: `%s`\n", str);
	return 0;
}

/*
	@brief 系统调用：等待事件。
	@param eax 系统调用号（应为 `SYSCALL_WAIT_EVENT`）
	@param ebx 存储事件 ID 的指针
	@param ecx 存储事件参数的指针
*/
static uint32_t syscall_wait_event(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();

	if (!(verify_user_pointer(task, ebx + task->data_base, sizeof(EVENT)) && verify_user_pointer(task, ecx + task->data_base, sizeof(uint32_t)))) {
		debug("SYSCALL: Invalid pointer passed to wait_event: 0x%08x\n", ebx);
		return (uint32_t) &(task->tss.esp0); /* 强制结束程序 */
	}

	for (;;) {
		cli();
		if (fifo_status(&task->fifo) == 0) {
			sti();
			task_sleep(task);
		} else {
			EVENT event;
			fifo_pop_event(&task->fifo, &event);
			sti();
			/* 将事件信息复制到用户空间 */
			*(uint32_t *) (ebx + task->data_base) = event.id;
			*(uint32_t *) (ecx + task->data_base) = event.param;
			return 0;
		}
	}

	return 0;
}

/*
	@brief 系统调用：创建窗口。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_CREATE`）
	@param ebx 窗口标题字符串
	@param ecx 窗口样式
	@param edx 窗口宽度和高度（高 16 位为宽度，低 16 位为高度）
	@return 窗口句柄
*/
static uint32_t syscall_window_create(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();

	const char *title = (const char *) (ebx + task->data_base);
	uint32_t style = ecx;
	uint16_t width = HIGH16(edx);
	uint16_t height = LOW16(edx);

	WINDOW *window = memory_alloc_irqsave(&g_mp, sizeof(WINDOW), task);
	if (!window) {
		debug("SYSCALL: Failed to allocate memory for window.\n");
		return 0; /* 内存分配失败，返回无效句柄 */
	}

	int32_t result = window_create(window, 0, 0, width, height, style, STARTUP_POS_CASCADE, title, task);
	if (result != WD_SUCCESS) {
		debug("SYSCALL: Failed to create window (error code: %d).\n", result);
		memory_free_irqsave(&g_mp, window);
		return 0; /* 窗口创建失败，返回无效句柄 */
	}

	// 暂时自动激活
	window_activate(window);

	HANDLE hwnd = handle_table_alloc(&task->hwnd_table, window, 0, (void (*)(void *)) &window_destroy);
	if (hwnd.value == 0) {
		debug("SYSCALL: Failed to allocate handle for window.\n");
		window_destroy(window);
		memory_free_irqsave(&g_mp, window);
		return 0; /* 句柄分配失败，返回无效句柄 */
	}

	debug("SYSCALL: Window created with handle 0x%08x.\n", hwnd.value);
	return hwnd.value; /* 返回窗口句柄 */
}

/*
	@brief 系统调用：在窗口客户区绘制矩形。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_DRAW_RECTANGLE`）
	@param ebx 窗口句柄
	@param ecx 矩形宽度和高度（高 16 位为宽度，低 16 位为高度）
	@param edx 矩形左上角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 矩形颜色
	@param edi 矩形边框宽度
*/
static uint32_t syscall_window_draw_rectangle(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_draw_rectangle");

	if (!window)
		return 0;

	client_draw_rectangle(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (uint16_t) HIGH16(ecx), (uint16_t) LOW16(ecx), (uint16_t) edi, (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区绘制矩形（通过角点坐标）。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_DRAW_RECTANGLE_BY_CORNERS`）
	@param ebx 窗口句柄
	@param ecx 矩形右下角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param edx 矩形左上角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 矩形颜色
	@param edi 矩形边框宽度
*/
static uint32_t syscall_window_draw_rectangle_by_corners(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_draw_rectangle_by_corners");

	if (!window)
		return 0;

	client_draw_rectangle_by_corners(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (int16_t) HIGH16(ecx), (int16_t) LOW16(ecx), (uint16_t) edi, (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区填充矩形。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_FILL_RECTANGLE`）
	@param ebx 窗口句柄
	@param ecx 矩形宽度和高度（高 16 位为宽度，低 16 位为高度）
	@param edx 矩形左上角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 矩形颜色
*/
static uint32_t syscall_window_fill_rectangle(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_fill_rectangle");

	if (!window)
		return 0;

	client_fill_rectangle(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (uint16_t) HIGH16(ecx), (uint16_t) LOW16(ecx), (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区填充矩形（通过角点坐标）。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_FILL_RECTANGLE_BY_CORNERS`）
	@param ebx 窗口句柄
	@param ecx 矩形右下角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param edx 矩形左上角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 矩形颜色
*/
static uint32_t syscall_window_fill_rectangle_by_corners(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_fill_rectangle_by_corners");

	if (!window)
		return 0;

	client_fill_rectangle_by_corners(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (int16_t) HIGH16(ecx), (int16_t) LOW16(ecx), (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区绘制直线。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_DRAW_LINE`）
	@param ebx 窗口句柄
	@param ecx 直线终点坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param edx 直线起点坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 直线颜色
*/
static uint32_t syscall_window_draw_line(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_draw_line");

	if (!window)
		return 0;

	client_draw_line(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (int16_t) HIGH16(ecx), (int16_t) LOW16(ecx), (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区绘制圆。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_DRAW_CIRCLE`）
	@param ebx 窗口句柄
	@param ecx 圆半径
	@param edx 圆心坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 圆颜色
*/
static uint32_t syscall_window_draw_circle(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_draw_circle");

	if (!window)
		return 0;

	client_draw_circle(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (int32_t) ecx, (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区填充圆。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_FILL_CIRCLE`）
	@param ebx 窗口句柄
	@param ecx 圆半径
	@param edx 圆心坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 圆颜色
*/
static uint32_t syscall_window_fill_circle(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_fill_circle");

	if (!window)
		return 0;

	client_fill_circle(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (int32_t) ecx, (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区绘制椭圆。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_DRAW_ELLIPSE`）
	@param ebx 窗口句柄
	@param ecx 椭圆宽度和高度（高 16 位为宽度，低 16 位为高度）
	@param edx 椭圆左上角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 椭圆颜色
*/
static uint32_t syscall_window_draw_ellipse(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_draw_ellipse");

	if (!window)
		return 0;

	client_draw_ellipse(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (uint16_t) HIGH16(ecx), (uint16_t) LOW16(ecx), (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区填充椭圆。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_FILL_ELLIPSE`）
	@param ebx 窗口句柄
	@param ecx 椭圆宽度和高度（高 16 位为宽度，低 16 位为高度）
	@param edx 椭圆左上角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 椭圆颜色
*/
static uint32_t syscall_window_fill_ellipse(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_fill_ellipse");

	if (!window)
		return 0;

	client_fill_ellipse(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (uint16_t) HIGH16(ecx), (uint16_t) LOW16(ecx), (COLOR) esi);
	return 0;
}

/*
	@brief 系统调用：在窗口客户区绘制 ASCII 字符串。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_DRAW_ASCII_STRING`）
	@param ebx 窗口句柄
	@param ecx 字符串颜色
	@param edx 字符串起点坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 字符串偏移量（相对于任务数据段基址）
	@param edi 字体编号
*/
static uint32_t syscall_window_draw_ascii_string(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_draw_ascii_string");

	if (!window || !verify_user_string(task, esi))
		return 0;

	client_draw_ascii_string(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (COLOR) ecx, (const char *) (task->data_base + esi), select_user_font(edi));
	return 0;
}

/*
	@brief 系统调用：在窗口客户区绘制 Unicode 字符串。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_DRAW_UNICODE_STRING`）
	@param ebx 窗口句柄
	@param ecx 字符串颜色
	@param edx 字符串起点坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
	@param esi 字符串偏移量（相对于任务数据段基址）
	@param edi 字体编号
*/
static uint32_t syscall_window_draw_unicode_string(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_draw_unicode_string");

	if (!window || !verify_user_string(task, esi))
		return 0;

	client_draw_unicode_string(window, (int16_t) HIGH16(edx), (int16_t) LOW16(edx), (COLOR) ecx, (const char *) (task->data_base + esi), select_user_font(edi));
	return 0;
}

/*
	@brief 系统调用：刷新窗口。
	@param eax 系统调用号（应为 `SYSCALL_WINDOW_REFRESH`）
	@param ebx 窗口句柄
	@param ecx 刷新区域宽度和高度（高 16 位为宽度，低 16 位为高度）
	@param edx 刷新区域左上角坐标（高 16 位为 X 坐标，低 16 位为 Y 坐标）
*/
static uint32_t syscall_window_refresh(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	WINDOW *window = lookup_user_window(task, ebx, "window_refresh");

	if (!window)
		return 0;

	int32_t x0 = (int16_t) HIGH16(edx) + window->client_x;
	int32_t y0 = (int16_t) LOW16(edx) + window->client_y;
	int32_t x1 = x0 + (uint16_t) HIGH16(ecx) + window->client_x;
	int32_t y1 = y0 + (uint16_t) LOW16(ecx) + window->client_y;
	layer_refresh(window->layer, x0, y0, x1, y1);

	return 0;
}

static syscall_handler_t syscall_handlers[] = {
	[SYSCALL_EXIT_PROCESS] = syscall_exit_process,
	[SYSCALL_DEBUG_PRINT] = syscall_debug_print,
	[SYSCALL_WAIT_EVENT] = syscall_wait_event,
	[SYSCALL_WINDOW_CREATE] = syscall_window_create,
	[SYSCALL_WINDOW_DRAW_RECTANGLE] = syscall_window_draw_rectangle,
	[SYSCALL_WINDOW_DRAW_RECTANGLE_BY_CORNERS] = syscall_window_draw_rectangle_by_corners,
	[SYSCALL_WINDOW_FILL_RECTANGLE] = syscall_window_fill_rectangle,
	[SYSCALL_WINDOW_FILL_RECTANGLE_BY_CORNERS] = syscall_window_fill_rectangle_by_corners,
	[SYSCALL_WINDOW_DRAW_LINE] = syscall_window_draw_line,
	[SYSCALL_WINDOW_DRAW_CIRCLE] = syscall_window_draw_circle,
	[SYSCALL_WINDOW_FILL_CIRCLE] = syscall_window_fill_circle,
	[SYSCALL_WINDOW_DRAW_ELLIPSE] = syscall_window_draw_ellipse,
	[SYSCALL_WINDOW_FILL_ELLIPSE] = syscall_window_fill_ellipse,
	[SYSCALL_WINDOW_DRAW_ASCII_STRING] = syscall_window_draw_ascii_string,
	[SYSCALL_WINDOW_DRAW_UNICODE_STRING] = syscall_window_draw_unicode_string,
	[SYSCALL_WINDOW_REFRESH] = syscall_window_refresh,
};

uint32_t system_call(registers_t *regs)
{
	uint32_t syscall_number = regs->eax;

	if (syscall_number >= sizeof(syscall_handlers) / sizeof(syscall_handlers[0]) || syscall_handlers[syscall_number] == NULL) {
		debug("SYSCALL: Invalid system call number: %d\n", syscall_number);
		return 0;
	}

	uint32_t result = syscall_handlers[regs->eax](regs->edi, regs->esi, regs->ebp, regs->esp, regs->ebx, regs->edx, regs->ecx, regs->eax);
	if (syscall_number == SYSCALL_EXIT_PROCESS) {
		/* 如果是退出程序的系统调用，返回栈顶地址以结束程序 */
		return result;
	} else {
		regs->eax = result; /* 将结果存储在 eax 寄存器中返回给应用程序 */
		return 0;
	}
}
