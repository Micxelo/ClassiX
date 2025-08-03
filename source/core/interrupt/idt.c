/*
	core/interrupt/idt.c
*/

#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>

typedef struct __attribute__((packed)) {
	uint16_t base_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t flags;
	uint16_t base_high;
} idt_entry_t;

typedef struct __attribute__((packed)) {
	uint16_t limit;
	uint32_t base;
} idt_ptr_t;

static idt_entry_t idt_entries[256] = { };
static idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint32_t ar)
{
	idt_entries[num].base_low = base & 0xffff;
	idt_entries[num].selector = selector;
	idt_entries[num].zero = 0;
	idt_entries[num].flags = ar & 0xff;
	idt_entries[num].base_high = (base >> 16) & 0xffff;
}

extern void asm_isr_keyboard(void);

void init_idt(void)
{
	/* 设置 IDT 指针 */
	idt_ptr.limit = sizeof(idt_entries) - 1;
	idt_ptr.base = (uint32_t) &idt_entries;

	/* 设置 IDT */
	idt_set_gate(0x21, (uint32_t) asm_isr_keyboard, 0x08, AR_INTGATE32);

	/* 加载 IDT */
	asm volatile ("lidt %0"::"m"(idt_ptr));
}
