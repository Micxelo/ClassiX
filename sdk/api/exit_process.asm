; api/exit.asm
; void __attribute__((noreturn)) exit_process(int32_t status);

%include "syscall.inc"

bits 32

global _cx_exit_process

section .text

_cx_exit_process:
	mov eax, SYS_EXIT_PROCESS
	mov ebx, [esp + 4]			; status
	int 0x40
	ret
