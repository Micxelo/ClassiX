;
;	core/int/inthandler.asm
;

section .text

; 键盘中断
global _asm_inthandler_21
extern _inthandler_21
_asm_inthandler_21:
	push es
	push ds
	pushad					; 保存所有通用寄存器
	mov eax, esp			; 将当前栈指针作为参数传递
	push eax
	mov ax, ss
	mov ds, ax				; 设置 DS = SS（内核数据段）
	mov es, ax				; 设置 ES = SS（内核数据段）
	call _inthandler_21		; 调用 C 中断处理函数
	pop eax					; 清理参数
	popad					; 恢复所有通用寄存器
	pop ds
	pop es
	iretd					; 32 位中断返回
