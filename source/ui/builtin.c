/*
	ui/builtin.c
*/

#include <ClassiX/builtin.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

const uint32_t builtin_cursor_arrow[] = {
#define T									0x00ffffff
#define B									0xff000000
#define W									0xffffffff
	B, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, 
	B, B, T, T, T, T, T, T, T, T, T, T, T, T, T, T, 
	B, W, B, T, T, T, T, T, T, T, T, T, T, T, T, T, 
	B, W, W, B, T, T, T, T, T, T, T, T, T, T, T, T, 
	B, W, W, W, B, T, T, T, T, T, T, T, T, T, T, T, 
	B, W, W, W, W, B, T, T, T, T, T, T, T, T, T, T, 
	B, W, W, W, W, W, B, T, T, T, T, T, T, T, T, T, 
	B, W, W, W, W, W, W, B, T, T, T, T, T, T, T, T, 
	B, W, W, W, W, W, W, W, B, T, T, T, T, T, T, T, 
	B, W, W, W, W, W, W, W, W, B, T, T, T, T, T, T, 
	B, W, W, W, W, W, W, W, W, W, B, T, T, T, T, T, 
	B, W, W, W, W, B, B, B, B, B, B, T, T, T, T, T, 
	B, W, W, W, B, T, T, T, T, T, T, T, T, T, T, T, 
	B, W, W, B, T, T, T, T, T, T, T, T, T, T, T, T, 
	B, W, B, T, T, T, T, T, T, T, T, T, T, T, T, T, 
	B, B, T, T, T, T, T, T, T, T, T, T, T, T, T, T, 
#undef W
#undef B
#undef T
};
