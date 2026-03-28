;
;	core/programs/exec.asm
;

global _program_api
_program_api:
	sti
	push ds
	push es
	pushad					; 保存寄存器值
	pushad					; 向 system_call 传值
	mov ax, ss
	mov ds, ax
	mov es, ax
	extern _system_call
	call _system_call
	cmp eax, 0
	jne _program_end		; 结束应用程序
	add esp, 32
	popad
	pop es
	pop ds
	iretd

global _program_end
_program_end:
	mov esp, [eax]
	mov dword [eax + 4], 0
	popad					; 恢复寄存器
	ret

; void program_start(uint32_t eip, uint32_t cs, uint32_t esp, uint32_t ds, uint32_t *tss_esp0);
global _program_start
_program_start:
	pushad					; 保存寄存器

	; 读取函数参数
	mov eax, [esp + 36]		; 应用程序 eip
	mov ecx, [esp + 40]		; 应用程序 cs
	mov edx, [esp + 44]		; 应用程序 esp
	mov ebx, [esp + 48]		; 应用程序 ds/ss
	mov ebp, [esp + 52]		; tss.esp0

	; 设置 TSS
	mov [ebp], esp			; 内核 esp
	mov word [ebp + 4], ss	; 内核 ess

	; 构造特权级切换栈帧
	push ebx				; 应用程序 ss
	push edx				; 应用程序 esp
	push dword 0x202		; 应用程序 eflags
	push ecx				; 应用程序 cs
	push eax				; 应用程序 eip

	or ecx, 3				; 设置 RPL=3
	or ebx, 3				; 设置 RPL=3

	; 设置用户态数据段寄存器
	mov es, bx
	mov ds, bx
	mov fs, bx
	mov gs, bx

	iretd
