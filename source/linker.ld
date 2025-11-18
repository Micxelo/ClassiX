/*
	linker.ld
	ClassiX Kernel Linking Script
*/

OUTPUT_FORMAT("binary")
ENTRY(start)

PHYS_BASE = 0x00100000;					/* 1 MB */

/* 内存布局 */
MEMORY {
	RAM (rwx) : ORIGIN = PHYS_BASE, LENGTH = 1M
}

SECTIONS {
	. = PHYS_BASE;
	load_phys = .;

	_kernel_start_phys = .;			/* 内核物理起始 */

	/* ClassiX 头 */
	.classix : ALIGN(1K) {
		*(classix)
	} > RAM

	/* Multiboot 头 */
	.multiboot : ALIGN(1K) {
		multiboot_header_phys = .;
		*(.multiboot)
	} > RAM

	/* 代码段 */
	.text : ALIGN(1K) {
		*(.text)
		*(.text.*)
	} > RAM

	/* 只读数据 */
	.rodata : ALIGN(1K) {
		*(.rodata)
		*(.rodata.*)
	} > RAM

	/* 已初始化数据 */
	.data : ALIGN(1K) {
		*(.data)
		*(.data.*)
	} > RAM

	load_end_phys = .;

	.bss : ALIGN(4K) {
		_bss_start_phys = .;			/* BSS 起始 */
		*(.bss)
		*(.bss.*)
		*(COMMON)
		_bss_end_phys = .;				/* BSS 终止 */
	} > RAM

	. = ALIGN(4K);
	_kernel_end_phys = .;				/* 内核物理结束 */
}

PROVIDE(entry_phys = start);
PROVIDE(kernel_size = _kernel_end_phys - _kernel_start_phys);
PROVIDE(bss_start_phys = _bss_start_phys);
PROVIDE(bss_end_phys = _bss_end_phys);
