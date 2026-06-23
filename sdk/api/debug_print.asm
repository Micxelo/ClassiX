; api/debug_print.asm
; void cx_debug_print(const char* str);

%include "syscall.inc"

bits 32

global _cx_debug_print

section .text
_cx_debug_print:
	mov eax, SYS_DEBUG_PRINT
	mov ebx, [esp + 4]			; str
	int 0x40
	ret
