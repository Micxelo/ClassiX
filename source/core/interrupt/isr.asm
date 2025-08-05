;
;	core/interrupt/isr.asm
;

section .text

; 中断处理（无错误号）宏
%macro ISR_TEMPLATE		1
global _asm_isr_%1
_asm_isr_%1:
	; 保存段寄存器
	push es
	push ds
	
	; 保存通用寄存器
	pushad
	
	; 设置内核数据段
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	
	; 构建参数结构体指针
	mov eax, esp
	push eax			; 压入参数指针
	extern _isr_%1
	call _isr_%1		; 调用 C 处理函数
	add esp, 4			; 清理参数
	
	; 恢复通用寄存器
	popad
	
	; 恢复段寄存器
	pop ds
	pop es
	
	; 中断返回
	iretd
%endmacro

; 使用宏定义各中断处理程序
ISR_TEMPLATE keyboard
ISR_TEMPLATE mouse
ISR_TEMPLATE pit
