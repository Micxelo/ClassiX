/*
	core/int/dsctable.c
*/

#include <ClassiX/typedef.h>
#include <ClassiX/int.h>

#define ADDR_IDT							0x0026f800
#define LIMIT_IDT							0x000007ff
#define ADDR_GDT							0x00270000
#define LIMIT_GDT							0x0000ffff
#define ADDR_BOTPAK							0x00280000
#define LIMIT_BOTPAK						0x0007ffff

#define AR_DATA32_RW						0x4092
#define AR_CODE32_ER						0x409a
#define AR_LDT								0x0082
#define AR_TSS32							0x0089
#define AR_INTGATE32						0x008e

typedef struct {
	int16_t limit_low, base_low;
	int8_t base_mid, access_right;
	int8_t limit_high, base_high;
} segment_descriptor_t;
typedef struct {
	uint16_t offset_low, selector;
	int8_t dw_count, access_right;
	uint16_t offset_high;
} gate_descriptor_t;

extern void asm_inthandler_21(void);

static inline void load_gdtr(uint16_t limit, uint32_t addr)
{
	struct {
		uint16_t limit;
		uint32_t addr;
	} __attribute__((packed)) gdtr = { limit, addr };

	asm volatile ("lgdt %0"::"m"(gdtr));
}

static inline void load_idtr(uint16_t limit, uint32_t base)
{
	struct {
		uint16_t limit;
		uint32_t base;
	} __attribute__((packed)) idtr = { limit, base };

	asm volatile ("lidt %0"::"m"(idtr));
}

static void set_segmdesc(segment_descriptor_t *sd, uint32_t limit, uint32_t base, int ar)
{
	if (limit > 0xfffff) {
		ar |= 0x8000; /* G_bit = 1 */
		limit /= 0x1000;
	}

	sd->limit_low    = limit & 0xffff;
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff;
	sd->access_right = ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;

	return;
}

static void set_gatedesc(gate_descriptor_t *gd, size_t offset, uint32_t selector, int ar)
{
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}

void init_gdtidt(void)
{
	segment_descriptor_t *gdt = (segment_descriptor_t *) ADDR_GDT;
	gate_descriptor_t *idt = (gate_descriptor_t *) ADDR_IDT;

	/* GDT 初始化 */
	for (int i = 0; i <= LIMIT_GDT / 8; i++) {
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	set_segmdesc(gdt + 1, 0xffffffff,   0x00000000,  AR_DATA32_RW);
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADDR_BOTPAK, AR_CODE32_ER);
	load_gdtr(LIMIT_GDT, ADDR_GDT);

	/* IDT 初始化 */
	for (int i = 0; i <= LIMIT_IDT / 8; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	load_idtr(LIMIT_IDT, ADDR_IDT);

	set_gatedesc(idt + 0x21, (int) asm_inthandler_21, 2 << 3, AR_INTGATE32); /* 键盘 */
	return;
}
