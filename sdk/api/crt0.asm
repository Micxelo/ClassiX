; api/crt0.asm

bits 32

extern _main

global _start

_start:
	call _main
.exit:
	mov ebx, eax	; main 函数返回值
	mov eax, 0		; exit_process
	int 0x40
.hang:
	jmp .hang
