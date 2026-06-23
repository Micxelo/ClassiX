; api/window_draw_rectangle_by_corners.asm
; void cx_window_draw_rectangle_by_corners(HANDLE hwnd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, COLOR color, uint16_t border_width);

%include "syscall.inc"

bits 32

global _cx_window_draw_rectangle_by_corners

section .text
_cx_window_draw_rectangle_by_corners:
	mov eax, SYS_WINDOW_DRAW_RECTANGLE_BY_CORNERS
	mov ebx, [esp + 4]			; hwnd
	mov edx, [esp + 8]			; x1
	shl edx, 16
	mov dx, [esp + 12]			; y1
	mov ecx, [esp + 16]			; x2
	shl ecx, 16
	mov cx, [esp + 20]			; y2
	mov esi, [esp + 24]			; color
	mov edi, [esp + 28]			; border_width
	int 0x40
	ret
