; api/window_draw_rectangle.asm
; void cx_window_draw_rectangle(HANDLE hwnd, int16_t x, int16_t y, uint16_t width, uint16_t height, COLOR color, uint16_t border_width);

%include "syscall.inc"

bits 32

global _cx_window_draw_rectangle

section .text
_cx_window_draw_rectangle:
	mov eax, SYS_WINDOW_DRAW_RECTANGLE
	mov ebx, [esp + 4]			; hwnd
	mov edx, [esp + 8]			; x
	shl edx, 16
	mov dx, [esp + 12]			; y
	mov ecx, [esp + 16]			; width
	shl ecx, 16
	mov cx, [esp + 20]			; height
	mov esi, [esp + 24]			; color
	mov edi, [esp + 28]			; border_width
	int 0x40
	ret
