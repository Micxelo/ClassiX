/*
	core/interrupt/exception.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

static void __attribute__((noreturn)) print_exception_info(const char *exception_name, ISR_PARAMS *params)
{
	debug("EXCEPTION: %s\n", exception_name);
	debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params->eip, params->cs, params->eflags);
	debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params->eax, params->ebx, params->ecx, params->edx);
	debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params->esp, params->ebp, params->esi, params->edi);
	debug("  DS: 0x%08x, ES: 0x%08x\n", params->ds, params->es);
	debug("  Error Code: 0x%08x\n", params->errcode);

	for(;;)
		asm volatile ("hlt");
}

/* 除零异常 */
void isr_de(ISR_PARAMS *params)
{
    print_exception_info("Divide Error", params);
}

/* 调试异常 */
void isr_db(ISR_PARAMS *params)
{
    print_exception_info("Debug Exception", params);
}

/* 非屏蔽中断异常 */
void isr_nmi(ISR_PARAMS *params)
{
    print_exception_info("Non-Maskable Interrupt", params);
}

/* 断点异常 */
void isr_bp(ISR_PARAMS *params)
{
    print_exception_info("Breakpoint", params);
}

/* 溢出异常 */
void isr_of(ISR_PARAMS *params)
{
    print_exception_info("Overflow", params);
}

/* 越界异常 */
void isr_br(ISR_PARAMS *params)
{
    print_exception_info("Bound Range Exceeded", params);
}

/* 无效操作码异常 */
void isr_ud(ISR_PARAMS *params)
{
    print_exception_info("Invalid Opcode", params);
}

/* 设备不可用异常 */
void isr_nm(ISR_PARAMS *params)
{
    print_exception_info("Device Not Available", params);
}

/* 双重故障异常 */
void isr_df(ISR_PARAMS *params)
{
    print_exception_info("Double Fault", params);
}

/* 任务状态段异常 */
void isr_ts(ISR_PARAMS *params)
{
    print_exception_info("Invalid TSS", params);
}

/* 段不存在异常 */
void isr_np(ISR_PARAMS *params)
{
    print_exception_info("Segment Not Present", params);
}

/* 栈段异常 */
void isr_ss(ISR_PARAMS *params)
{
    print_exception_info("Stack-Segment Fault", params);
}

/* 一般保护异常 */
void isr_gp(ISR_PARAMS *params)
{
    print_exception_info("General Protection Fault", params);
}

/* 页错误异常 */
void isr_pf(ISR_PARAMS *params)
{
    print_exception_info("Page Fault", params);
}

/* x87 浮点异常 */
void isr_mf(ISR_PARAMS *params)
{
    print_exception_info("x87 Floating-Point Exception", params);
}

/* 对齐检查异常 */
void isr_ac(ISR_PARAMS *params)
{
    print_exception_info("Alignment Check", params);
}
