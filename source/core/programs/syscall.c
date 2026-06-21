/*
	core/programs/syscall.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/programs.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>

#define LOW8(u16) 							(uint8_t) ((u16) & 0x00ff)
#define HIGH8(u16) 							(uint8_t) (((u16) & 0xff00) >> 8)
#define LOW16(u32) 							(uint16_t) ((u32) & 0x0000ffff)
#define HIGH16(u32) 						(uint16_t) (((u32) & 0xffff0000) >> 16)

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

typedef uint32_t (*syscall_handler_t)(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax);

typedef enum {
	SYSCALL_EXIT_PROCESS = 0,	/* 退出程序 */
	SYSCALL_DEBUG_PRINT,		/* 调试输出 */
	SYSCALL_WAIT_EVENT,			/* 等待事件 */
	SYSCALL_CREATE_WINDOW,		/* 创建窗口 */
} SYSCALL_NUMBER;

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
	@brief 
*/
static uint32_t syscall_create_window(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	uint32_t *registers = &eax + 1; /* regsters[7] = eax */

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

	registers[7] = (uint32_t) window; /* 返回窗口指针作为句柄 */

	return 0;
}

static syscall_handler_t syscall_handlers[] = {
	[SYSCALL_EXIT_PROCESS] = syscall_exit_process,
	[SYSCALL_DEBUG_PRINT] = syscall_debug_print,
	[SYSCALL_WAIT_EVENT] = syscall_wait_event,
	[SYSCALL_CREATE_WINDOW] = syscall_create_window,
};

uint32_t system_call(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	if (eax >= sizeof(syscall_handlers) / sizeof(syscall_handlers[0]) || syscall_handlers[eax] == NULL) {
		debug("Invalid system call number: %d\n", eax);
		return 0;
	}
	return syscall_handlers[eax](edi, esi, ebp, esp, ebx, edx, ecx, eax);
}
