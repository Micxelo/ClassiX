; api/wait_event.asm
; int32_t cx_wait_event(EVENT* event, uint32_t* param);

%include "syscall.inc"

bits 32

global _cx_wait_event

section .text
_cx_wait_event:
	mov eax, SYS_WAIT_EVENT
	mov ebx, [esp + 4]			; event
	mov ecx, [esp + 8]			; param
	int 0x40
	ret
