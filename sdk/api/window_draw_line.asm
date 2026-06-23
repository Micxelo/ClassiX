; api/window_draw_line.asm
; void cx_window_draw_line(HANDLE hwnd, int16_t start_x, int16_t start_y, int16_t end_x, int16_t end_y, COLOR color);

%include "syscall.inc"

bits 32

global _cx_window_draw_line

section .text
_cx_window_draw_line:
	mov eax, SYS_WINDOW_DRAW_LINE
	mov ebx, [esp + 4]			; hwnd
	mov edx, [esp + 8]			; start_x
	shl edx, 16
	mov dx, [esp + 12]			; start_y
	mov ecx, [esp + 16]			; end_x
	shl ecx, 16
	mov cx, [esp + 20]			; end_y
	mov esi, [esp + 24]			; color
	int 0x40
	ret
