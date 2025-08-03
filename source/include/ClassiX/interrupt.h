/*
	include/ClassiX/interrupt.h
*/

#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define AR_0_CODE32_ER						0xcf9a
#define AR_0_DATA32_RW						0xcf92
#define AR_3_CODE32_ER						0xcffa
#define AR_3_DATA32_RW						0xcff2

#define AR_LDT								0x0082
#define AR_TSS32							0x0089
#define AR_INTGATE32						0x008e

#define PIC0_ICW1							0x0020
#define PIC0_OCW2							0x0020
#define PIC0_IMR							0x0021
#define PIC0_ICW2							0x0021
#define PIC0_ICW3							0x0021
#define PIC0_ICW4							0x0021
#define PIC1_ICW1							0x00a0
#define PIC1_OCW2							0x00a0
#define PIC1_IMR							0x00a1
#define PIC1_ICW2							0x00a1
#define PIC1_ICW3							0x00a1
#define PIC1_ICW4							0x00a1

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint32_t ar);

void init_gdt(void);
void init_idt(void);
void init_pic(void);

#ifdef __cplusplus
	}
#endif

#endif
