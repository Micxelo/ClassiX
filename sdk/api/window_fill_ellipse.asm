; api/window_fill_ellipse.asm
; void cx_window_fill_ellipse(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height, COLOR color);

%include "syscall.inc"

bits 32

global _cx_window_fill_ellipse

section .text
_cx_window_fill_ellipse:
	mov eax, SYS_WINDOW_FILL_ELLIPSE
	mov ebx, [esp + 4]			; hwnd
	mov edx, [esp + 8]			; x
	shl edx, 16
	mov dx, [esp + 12]			; y
	mov ecx, [esp + 16]			; width
	shl ecx, 16
	mov cx, [esp + 20]			; height
	mov esi, [esp + 24]			; color
	int 0x40
	ret