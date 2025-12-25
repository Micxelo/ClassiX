;
;	core/interrupt/isr.asm
;

section .text

; 中断处理（无错误号）宏
%macro ISR_TEMPLATE			1
global _asm_isr_%1
_asm_isr_%1:
	; 错误码占位
	push 0

	; 此时栈布局
	; +--------------+
	; |     eip      | <--- esp + 0
	; +--------------+
	; |      cs      | <--- esp + 4
	; +--------------+
	; |    eflags    | <--- esp + 8
	; +--------------+
	; |   errcode    | <--- esp + 12
	; +--------------+

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
	push esp				; 压入参数指针
	extern _isr_%1
	call _isr_%1			; 调用 C 处理函数
	add esp, 4				; 清理参数
	
	; 恢复通用寄存器
	popad
	
	; 恢复段寄存器
	pop ds
	pop es

	; 清理错误码
	add esp, 4
	
	; 中断返回
	iretd
%endmacro

; 中断处理（有错误号）宏
%macro ISR_ERRCODE_TEMPLATE		1
global _asm_isr_%1
_asm_isr_%1:
	; 此时栈布局
	; +--------------+
	; |     eip      | <--- esp + 0
	; +--------------+
	; |      cs      | <--- esp + 4
	; +--------------+
	; |    eflags    | <--- esp + 8
	; +--------------+
	; |   errcode    | <--- esp + 12
	; +--------------+	

	; 保存段寄存器
	push es
	push ds
	
	; 保存通用寄存器
	pushad
	
	; 设置内核数据段
	mov ax, ss
	mov ds, ax
	mov es, ax
	
	; 构建参数结构体指针
	push esp				; 压入参数指针
	extern _isr_%1
	call _isr_%1			; 调用 C 处理函数
	add esp, 4				; 清理参数
	
	; 恢复通用寄存器
	popad
	
	; 恢复段寄存器
	pop ds
	pop es
	
	; 清理错误码
	add esp, 4
	
	; 中断返回
	iretd
%endmacro

; 使用宏定义各中断处理程序
ISR_TEMPLATE pit			; PIT 中断
ISR_TEMPLATE keyboard		; 键盘中断
ISR_TEMPLATE mouse			; 鼠标中断

ISR_TEMPLATE de				; 除零异常
ISR_TEMPLATE db				; 调试异常
ISR_TEMPLATE nmi			; 非屏蔽中断异常
ISR_TEMPLATE bp				; 断点异常
ISR_TEMPLATE of				; 溢出异常
ISR_TEMPLATE br				; 越界异常
ISR_TEMPLATE ud				; 无效操作码异常
ISR_TEMPLATE nm				; 设备不可用异常
ISR_ERRCODE_TEMPLATE df		; 双重故障异常
ISR_ERRCODE_TEMPLATE ts		; 任务状态段异常
ISR_ERRCODE_TEMPLATE np		; 段不存在异常
ISR_ERRCODE_TEMPLATE ss		; 栈段异常
ISR_ERRCODE_TEMPLATE gp		; 一般保护异常
ISR_ERRCODE_TEMPLATE pf		; 页错误异常
ISR_TEMPLATE mf				; x87 浮点异常
ISR_ERRCODE_TEMPLATE ac		; 对齐检查异常

; void farjmp(uint32_t eip, uint32_t cs)
global _farjmp
_farjmp:
	jmp far [esp + 4]
	ret

; void farcall(uint32_t eip, uint32_t cs)
global _farcall
_farcall:
	call far [esp + 4]
	ret
