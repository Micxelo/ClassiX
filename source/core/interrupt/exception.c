/*
	core/interrupt/exception.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

/* 除零异常 */
void isr_de(ISR_PARAMS params)
{
	debug("EXCEPTION: Divide Error\n");
	debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
	debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
	debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
	debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

	for(;;)
		asm volatile ("hlt");
}

/* 调试异常 */
void isr_db(ISR_PARAMS params)
{
    debug("EXCEPTION: Debug Exception\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 非屏蔽中断异常 */
void isr_nmi(ISR_PARAMS params)
{
    debug("EXCEPTION: Non-Maskable Interrupt\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 断点异常 */
void isr_bp(ISR_PARAMS params)
{
    debug("EXCEPTION: Breakpoint\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 溢出异常 */
void isr_of(ISR_PARAMS params)
{
    debug("EXCEPTION: Overflow\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 越界异常 */
void isr_br(ISR_PARAMS params)
{
    debug("EXCEPTION: Bound Range Exceeded\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 无效操作码异常 */
void isr_ud(ISR_PARAMS params)
{
    debug("EXCEPTION: Invalid Opcode\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 设备不可用异常 */
void isr_nm(ISR_PARAMS params)
{
    debug("EXCEPTION: Device Not Available\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 双重故障异常 */
void isr_df(ISR_PARAMS params)
{
    debug("EXCEPTION: Double Fault\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 任务状态段异常 */
void isr_ts(ISR_PARAMS params)
{
    debug("EXCEPTION: Invalid TSS\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 段不存在异常 */
void isr_np(ISR_PARAMS params)
{
    debug("EXCEPTION: Segment Not Present\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 栈段异常 */
void isr_ss(ISR_PARAMS params)
{
    debug("EXCEPTION: Stack-Segment Fault\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 一般保护异常 */
void isr_gp(ISR_PARAMS params)
{
	debug("EXCEPTION: General Protection Fault\n");
	debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
	debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
	debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
	debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

	for(;;)
		asm volatile ("hlt");
}

/* 页错误异常 */
void isr_pf(ISR_PARAMS params)
{
    debug("EXCEPTION: Page Fault\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* x87 浮点异常 */
void isr_mf(ISR_PARAMS params)
{
    debug("EXCEPTION: x87 Floating-Point Exception\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}

/* 对齐检查异常 */
void isr_ac(ISR_PARAMS params)
{
    debug("EXCEPTION: Alignment Check\n");
    debug("  EIP: 0x%08x, CS: 0x%08x, EFLAGS: 0x%08x\n", params.eip, params.cs, params.eflags);
    debug("  EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n", params.eax, params.ebx, params.ecx, params.edx);
    debug("  ESP: 0x%08x, EBP: 0x%08x, ESI: 0x%08x, EDI: 0x%08x\n", params.esp, params.ebp, params.esi, params.edi);
    debug("  DS: 0x%08x, ES: 0x%08x\n", params.ds, params.es);

    for(;;)
        asm volatile ("hlt");
}
