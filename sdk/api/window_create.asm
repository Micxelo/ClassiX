; api/create_window.asm
; HANDLE cx_window_create(const char* title, uint32_t style, uint16_t width, uint16_t height);

%include "syscall.inc"

bits 32

global _cx_window_create

section .text
_cx_window_create:
	mov eax, SYS_WINDOW_CREATE
	mov ebx, [esp + 4]			; title
	mov ecx, [esp + 8]			; style
	mov edx, [esp + 12]			; width
	shl edx, 16
	mov dx, [esp + 16]			; height
	int 0x40
	ret
