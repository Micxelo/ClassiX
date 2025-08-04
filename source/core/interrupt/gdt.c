/*
	core/interrupt/gdt.c
*/

#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>

typedef struct __attribute__((packed)) {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} gdt_entry_t;

typedef struct __attribute__((packed)) {
	uint16_t limit;
	uint32_t base;
} gdt_ptr_t;

/*
	0 - reserved
	1 - kernel code segment
	2 - kernel data segment
	3 - user code segment
	4 - user data segment
	5 - tss
*/
static gdt_entry_t gdt_entries[6];
static gdt_ptr_t gdt_ptr;

/*
	@brief 设置 GDT 描述符。
	@param num 描述符编号
	@param base 段基址
	@param limit 段界限
	@param access 访问权限
	@param gran 粒度
*/
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
	gdt_entries[num].limit_low = (limit & 0xffff);
	gdt_entries[num].base_low = (base & 0xffff);
	gdt_entries[num].base_mid = (base >> 16) & 0xff;
	gdt_entries[num].access = access;
	gdt_entries[num].granularity = (limit >> 16) & 0x0f;
	gdt_entries[num].granularity |= ((gran << 4) & 0xf0);
	gdt_entries[num].base_high = (base >> 24) & 0xff;
}

/*
	@brief 初始化 GDT。
*/
void init_gdt(void)
{
	/* 设置 GDT 指针 */
	gdt_ptr.limit = sizeof(gdt_entries) - 1;
	gdt_ptr.base = (uint32_t) &gdt_entries;

	/* 设置 GDT */
	gdt_set_gate(0, 0, 0,          0,                     0                  );	/* 空描述符 */
	gdt_set_gate(1, 0, 0xffffffff, AR_0_CODE32_ER & 0xff, AR_0_CODE32_ER >> 8); /* 内核代码段 */
	gdt_set_gate(2, 0, 0xffffffff, AR_0_DATA32_RW & 0xff, AR_0_DATA32_RW >> 8);	/* 内核数据段 */
	gdt_set_gate(3, 0, 0xffffffff, AR_3_CODE32_ER & 0xff, AR_3_CODE32_ER >> 8);	/* 用户代码段 */
	gdt_set_gate(4, 0, 0xffffffff, AR_3_DATA32_RW & 0xff, AR_3_DATA32_RW >> 8);	/* 用户数据段 */

	/* 加载 GDT */
	asm volatile ("lgdt %0"::"m"(gdt_ptr));

	/* 刷新段寄存器 */
	asm volatile(
		"movw $0x10, %ax   \n"	/* 0x10 - 内核数据段选择子 */
		"movw %ax, %ds     \n"
		"movw %ax, %es     \n"
		"movw %ax, %fs     \n"
		"movw %ax, %gs     \n"
		"movw %ax, %ss     \n"
		"ljmp $0x08, $flush\n"	/* 0x08 - 内核代码段选择子 */
		"flush:");
}
