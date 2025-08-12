/*
	include/ClassiX/interrupt.h
*/

#ifndef _CLASSIX_INTERRUPT_H_
#define _CLASSIX_INTERRUPT_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define AR_0_CODE32_ER						(0xcf9a)
#define AR_0_DATA32_RW						(0xcf92)
#define AR_3_CODE32_ER						(0xcffa)
#define AR_3_DATA32_RW						(0xcff2)

#define GDT_LIMIT							(1024)
#define TASK_GDT0							(3)

#define AR_LDT								(0x0082)
#define AR_TSS32							(0x0089)
#define AR_INTGATE32						(0x008e)

#define PIC0_ICW1							(0x0020)
#define PIC0_OCW2							(0x0020)
#define PIC0_IMR							(0x0021)
#define PIC0_ICW2							(0x0021)
#define PIC0_ICW3							(0x0021)
#define PIC0_ICW4							(0x0021)
#define PIC1_ICW1							(0x00a0)
#define PIC1_OCW2							(0x00a0)
#define PIC1_IMR							(0x00a1)
#define PIC1_ICW2							(0x00a1)
#define PIC1_ICW3							(0x00a1)
#define PIC1_ICW4							(0x00a1)


typedef struct {
	/* 由处理器自动压入的部分 */
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	
	/* 由中断处理程序压入的部分 */
	uint32_t es;
	uint32_t ds;
	
	/* 通用寄存器 (PUSHAD 顺序) */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;	/* 中断前的 ESP 值 */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
} ISR_PARAMS;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint32_t ar);

void init_gdt(void);
void init_idt(void);
void init_pic(void);

extern void farjmp(uint32_t eip, uint32_t cs);
extern void farcall(uint32_t eip, uint32_t cs);

#ifdef __cplusplus
	}
#endif

#endif
