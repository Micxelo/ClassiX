/*
	core/programs/syscall.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/memory.h>
#include <ClassiX/programs.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>

#define LOW8(u16) 							(uint8_t) ((u16) & 0x00ff)
#define HIGH8(u16) 							(uint8_t) (((u16) & 0xff00) >> 8)
#define LOW16(u32) 							(uint16_t) ((u32) & 0x0000ffff)
#define HIGH16(u32) 						(uint16_t) (((u32) & 0xffff0000) >> 16)

typedef uint32_t (*syscall_handler_t)(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax);

typedef enum {
	SYSCALL_EXIT = 0,			/* 结束程序 */
} SYSCALL_NUMBER;

/*
	@brief 系统调用：退出程序。
	@param eax 系统调用号（应为 `SYSCALL_EXIT`）
	@param ebx 退出代码
	@return 栈顶地址，供 `program_end` 结束程序。
*/
static uint32_t syscall_exit(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	TASK *task = task_get_current();
	debug("SYSCALL: Program %d called exit with code %d.\n", TID(task), ebx);
	return (uint32_t) &(task->tss.esp0);
}

static syscall_handler_t syscall_handlers[] = {
	[SYSCALL_EXIT] = syscall_exit
};

uint32_t system_call(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax)
{
	if (eax >= sizeof(syscall_handlers) / sizeof(syscall_handlers[0]) || syscall_handlers[eax] == NULL) {
		debug("Invalid system call number: %d\n", eax);
		return 0;
	}
	return syscall_handlers[eax](edi, esi, ebp, esp, ebx, edx, ecx, eax);
}
