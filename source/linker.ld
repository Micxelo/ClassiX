ENTRY(start)

PHYS_BASE = 0x00100000;					/* 1 M */

SECTIONS {
	. = PHYS_BASE;

	_kernel_start_phys = .;				/* 内核物理起始 */

	.text BLOCK(4K) : ALIGN(4K) {
		*(.multiboot)
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K) {
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K) {
		*(.data)
	}

	.bss BLOCK(4K) : ALIGN(4K) {
		_bss_start = .;				/* BSS 起始 */
		*(COMMON)
		*(.bss)
		_bss_end = .;				/* BSS 终止 */
	}

	. = ALIGN(4K);
	_kernel_end_phys = .;			/* 内核物理结束 */
}
