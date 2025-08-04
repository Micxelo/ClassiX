;
;	core/interrupt/isr.asm
;

section .text

; PIT 中断
global _asm_isr_pit
extern _isr_pit
_asm_isr_pit:
	push es
	push ds
	pushad					; 保存所有通用寄存器
	mov eax, esp			; 将当前栈指针作为参数传递
	push eax
	mov ax, 0x10			; 内核数据段选择子
	mov ds, ax				; 设置 DS = SS（内核数据段）
	mov es, ax				; 设置 ES = SS（内核数据段）
	call _isr_pit			; 调用 C 中断处理函数
	pop eax					; 清理参数
	popad					; 恢复所有通用寄存器
	pop ds
	pop es
	iretd					; 32 位中断返回

; 键盘中断
global _asm_isr_keyboard
extern _isr_keyboard
_asm_isr_keyboard:
	push es
	push ds
	pushad					; 保存所有通用寄存器
	mov eax, esp			; 将当前栈指针作为参数传递
	push eax
	mov ax, 0x10			; 内核数据段选择子
	mov ds, ax				; 设置 DS = SS（内核数据段）
	mov es, ax				; 设置 ES = SS（内核数据段）
	call _isr_keyboard		; 调用 C 中断处理函数
	pop eax					; 清理参数
	popad					; 恢复所有通用寄存器
	pop ds
	pop es
	iretd					; 32 位中断返回

; 鼠标中断
global _asm_isr_mouse
extern _isr_mouse
_asm_isr_mouse:
	push es
	push ds
	pushad					; 保存所有通用寄存器
	mov eax, esp			; 将当前栈指针作为参数传递
	push eax
	mov ax, 0x10			; 内核数据段选择子
	mov ds, ax				; 设置 DS = SS（内核数据段）
	mov es, ax				; 设置 ES = SS（内核数据段）
	call _isr_mouse			; 调用 C 中断处理函数
	pop eax					; 清理参数
	popad					; 恢复所有通用寄存器
	pop ds
	pop es
	iretd					; 32 位中断返回
