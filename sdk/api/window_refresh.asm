; api/window_refresh.asm
; void cx_window_refresh(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height);

%include "syscall.inc"

bits 32

global _cx_window_refresh

section .text
_cx_window_refresh:
	mov eax, SYS_WINDOW_REFRESH
	mov ebx, [esp + 4]			; hwnd
	mov edx, [esp + 8]			; x
	shl edx, 16
	mov dx, [esp + 12]			; y
	mov ecx, [esp + 16]			; width
	shl ecx, 16
	mov cx, [esp + 20]			; height
	int 0x40
	ret
