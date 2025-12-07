;
;	core/boot.asm
;

; ClassiX Kernel 签名
CLASSIX						equ			0xcd59f115				; 魔数
VERSION						equ			0x00010000				; 版本号

; Multiboot Flags 位定义
MBALIGN						equ			1 << 0					; 对齐信息
MEMINFO						equ			1 << 1					; 内存映射信息
VIDEOMODE					equ			1 << 2					; 视频信息
ADDRWORD					equ			1 << 16					; 地址字段

; Multiboot Header 常量定义
MBFLAGS						equ			MBALIGN | MEMINFO | VIDEOMODE | ADDRWORD
MAGIC						equ			0x1badB002				; Multiboot 魔数
CHECKSUM					equ			-(MAGIC + MBFLAGS)		; 校验和
																; CHECKSUM + MAGIC + MBFLAGS 应等于零

; Multiboot Info 常量定义
MBI_MAGIC					equ			0x2BADB002				; Multiboot 信息魔数
MBI_MEMORY					equ			0x00000001				; 是否有基本的低/高端内存信息
MBI_BOOTDEV					equ			0x00000002				; 是否设置了启动设备
MBI_FRAMEBUFFER				equ			0x00001000				; 是否有帧缓冲区信息

section	.classix
align 4
classix:
	; ClassiX Header
	extern kernel_size
	.magic:			dd CLASSIX					; 签名
	.version:		dd VERSION					; 版本信息
	.entry:			dd start					; 入口点
	.size:			dd kernel_size				; 内核大小
	.reserved:		times 12 dd 0

; GRUB 在内核文件的前 8 KiB（按 32 位边界对齐）中搜索 Header
section .multiboot
align 4
multiboot:
	; Multiboot Header
	.magic			dd MAGIC
	.flags			dd MBFLAGS
	.checksum		dd CHECKSUM

	extern multiboot_header_phys
	extern load_phys
	extern load_end_phys
	extern bss_end_phys
	extern entry_phys

	; 地址字段
	.header_phys:	dd multiboot_header_phys	; Multiboot 头
	.load_phys:		dd load_phys				; 加载段起始
	.load_end_phys:	dd load_end_phys			; 加载段结束
	.bss_end_phys:	dd bss_end_phys				; BSS 段结束
	.entry_phys:	dd entry_phys				; 入口点

	; 视频模式
	dd 0										; 线性帧缓冲区
	dd 0										; 宽度（默认）
	dd 0										; 高度（默认）
	dd 0										; 色深（默认）

; linker.ld 指定 start 为内核入口点
section .text
align 4
global start
start:
	; GRUB 已切换至 32 位保护模式、禁用中断和分页
	
	; 清零 BSS
	pushad
	extern bss_start_phys
	extern bss_end_phys
	cld											; 清除方向标志
	mov edi, bss_start_phys						; BSS 段起始地址
	mov ecx, bss_end_phys						; BSS 段结束地址
	sub ecx, edi
	xor eax, eax
	rep stosb
	popad

	; 将 ESP 指向栈顶
	mov esp, stack_top

	; 重置 EFLAGS
	push dword 0
	popf

	; 传递 main 参数
	push ebx									; 指向 Multiboot 信息结构体的指针

	; 验证 Multiboot 魔数
	cmp eax, MBI_MAGIC
	jne .exit

	; 检查内存信息
	test dword [ebx], MBI_MEMORY
	jz .exit

	; 检查启动设备信息
	test dword [ebx], MBI_BOOTDEV
	jz .exit

	; 检查帧缓冲区信息
	test dword [ebx], MBI_FRAMEBUFFER
	jz .exit

	extern _main
	call _main									; 跳转至 C 入口点

.exit:
	; 从 main 返回，锁死 CPU
	cli											; 禁用中断
.hang:
	hlt
	jmp .hang
.end:

; Multiboot 标准未定义栈指针（ESP）的值，需由内核提供栈空间
; x86 栈必须 16 字节对齐
section .bss
align 16
stack_bottom:
	resb 0x10000								; 保留 1 MiB 栈空间
stack_top:
