; api/window_draw_unicode_string.asm
; void cx_window_draw_unicode_string(HANDLE hwnd, int16_t x, int16_t y, COLOR color, const char* string, uint32_t font_id);

%include "syscall.inc"

bits 32

global _cx_window_draw_unicode_string

section .text
_cx_window_draw_unicode_string:
	mov eax, SYS_WINDOW_DRAW_UNICODE_STRING
	mov ebx, [esp + 4]			; hwnd
	mov edx, [esp + 8]			; x
	shl edx, 16
	mov dx, [esp + 12]			; y
	mov ecx, [esp + 16]			; color
	mov esi, [esp + 20]			; string
	mov edi, [esp + 24]			; font_id
	int 0x40
	ret