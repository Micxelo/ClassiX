;
;	core/boot.asm
;

MBALIGN		equ			1 << 0				; 对齐信息
MEMINFO		equ			1 << 1				; 内存映射信息
VIDEOMODE	equ			1 << 2				; 视频信息

MBFLAGS		equ			MBALIGN | MEMINFO | VIDEOMODE
MAGIC		equ			0x1badB002			; Multiboot 魔数
CHECKSUM	equ			-(MAGIC + MBFLAGS)	; 上述值的校验和
											; CHECKSUM + MAGIC + MBFLAGS 应等于零


; GRUB 在内核文件的前 8 KiB（按 32 位边界对齐）中搜索 Header
section .multiboot
align 4
	; Multiboot Header
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM

	; ELF
	dd 0
	dd 0
	dd 0
	dd 0
	dd 0

	; 视频模式
	dd 0
	dd 0
	dd 0
	dd 0

; Multiboot 标准未定义栈指针（ESP）的值，需由内核提供栈空间
; x86 栈必须16字节对齐
section .bss
align 16
stack_bottom:
	resb 0x10000			; 保留 1 MiB 栈空间
stack_top:

; linker.ld 指定 start 为内核入口点
section .text
global start: function (start.end - start)
start:
	; GRUB 已切换至 32 位保护模式、禁用中断和分页

	mov esp, stack_top		; 将 ESP 指向栈顶

	; 重置 EFLAGS
	push long 0
	popf

	; 传递 main 参数
	push long ebx			; 指向 Multiboot 信息结构体的指针
	push long eax			; Multiboot 魔数

	extern _main
	call _main				; 跳转至 C 入口点

	; 从 main 返回，锁死计算机
	cli						; 禁用中断
.hang:
	hlt
	jmp .hang
.end:
