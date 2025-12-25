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

static idt_entry_t idt_entries[IDT_LIMIT];
static idt_ptr_t idt_ptr;

/*
	@brief 设置 IDT 描述符。
	@param num 描述符编号
	@param base 中断处理函数地址
	@param selector 段选择子
	@param ar 访问权限和属性
*/
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint32_t ar)
{
	idt_entries[num].base_low = base & 0xffff;
	idt_entries[num].selector = selector;
	idt_entries[num].zero = 0;
	idt_entries[num].flags = ar & 0xff;
	idt_entries[num].base_high = (base >> 16) & 0xffff;
}

void init_idt(void)
{
	/* 设置 IDT 指针 */
	idt_ptr.limit = sizeof(idt_entries) - 1;
	idt_ptr.base = (uint32_t) &idt_entries;

	/* 设置 IDT */
	for (int32_t i = 0; i < IDT_LIMIT; i++)
		idt_set_gate(i, 0, 0, 0);

	/* 加载 IDT */
	asm volatile ("lidt %0"::"m"(idt_ptr));

	/* 注册异常 IRQ */
	extern void asm_isr_de(void);
	extern void asm_isr_db(void);
	extern void asm_isr_nmi(void);
	extern void asm_isr_bp(void);
	extern void asm_isr_of(void);
	extern void asm_isr_br(void);
	extern void asm_isr_ud(void);
	extern void asm_isr_nm(void);
	extern void asm_isr_df(void);
	extern void asm_isr_ts(void);
	extern void asm_isr_np(void);
	extern void asm_isr_ss(void);
	extern void asm_isr_gp(void);
	extern void asm_isr_pf(void);
	extern void asm_isr_mf(void);
	extern void asm_isr_ac(void);

	idt_set_gate(0x00, (uint32_t) asm_isr_de,  0x08, AR_INTGATE32);	/* 除零异常 */
	idt_set_gate(0x01, (uint32_t) asm_isr_db,  0x08, AR_INTGATE32);	/* 调试异常 */
	idt_set_gate(0x02, (uint32_t) asm_isr_nmi, 0x08, AR_INTGATE32);	/* 非屏蔽中断异常 */
	idt_set_gate(0x03, (uint32_t) asm_isr_bp,  0x08, AR_INTGATE32); /* 断点异常 */
	idt_set_gate(0x04, (uint32_t) asm_isr_of,  0x08, AR_INTGATE32);	/* 溢出异常 */
	idt_set_gate(0x05, (uint32_t) asm_isr_br,  0x08, AR_INTGATE32);	/* 越界异常 */
	idt_set_gate(0x06, (uint32_t) asm_isr_ud,  0x08, AR_INTGATE32);	/* 无效操作码异常 */
	idt_set_gate(0x07, (uint32_t) asm_isr_nm,  0x08, AR_INTGATE32);	/* 设备不可用异常 */
	idt_set_gate(0x08, (uint32_t) asm_isr_df,  0x08, AR_INTGATE32);	/* 双重故障异常 */
	idt_set_gate(0x0A, (uint32_t) asm_isr_ts,  0x08, AR_INTGATE32);	/* 任务状态段异常 */
	idt_set_gate(0x0B, (uint32_t) asm_isr_np,  0x08, AR_INTGATE32);	/* 段不存在异常 */
	idt_set_gate(0x0C, (uint32_t) asm_isr_ss,  0x08, AR_INTGATE32);	/* 栈段异常 */
	idt_set_gate(0x0D, (uint32_t) asm_isr_gp,  0x08, AR_INTGATE32);	/* 一般保护异常 */
	idt_set_gate(0x0E, (uint32_t) asm_isr_pf,  0x08, AR_INTGATE32);	/* 页错误异常 */
	idt_set_gate(0x10, (uint32_t) asm_isr_mf,  0x08, AR_INTGATE32);	/* x87 浮点异常 */
	idt_set_gate(0x11, (uint32_t) asm_isr_ac,  0x08, AR_INTGATE32);	/* 对齐检查异常 */
}
