; api/window_draw_circle.asm
; void cx_window_draw_circle(HANDLE hwnd, int16_t center_x, int16_t center_y, int16_t radius, COLOR color);

%include "syscall.inc"

bits 32

global _cx_window_draw_circle

section .text
_cx_window_draw_circle:
	mov eax, SYS_WINDOW_DRAW_CIRCLE
	mov ebx, [esp + 4]			; hwnd
	mov edx, [esp + 8]			; center_x
	shl edx, 16
	mov dx, [esp + 12]			; center_y
	mov ecx, [esp + 16]			; radius
	mov esi, [esp + 20]			; color
	int 0x40
	ret
