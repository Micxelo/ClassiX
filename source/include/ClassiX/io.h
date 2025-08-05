/*
	include/ClassiX/io.h
*/

#ifndef _CLASSIX_IO_H_
#define _CLASSIX_IO_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define hlt()								asm volatile ("hlt")
#define cli()								asm volatile ("cli")
#define sti()								asm volatile ("sti")
#define nop()								asm volatile ("nop")
#define pause()								asm volatile ("pause")

#define out8(port, data)					asm volatile ("outb %%al, %%dx"::"a"(data), "d"(port))
#define out16(port, data)					asm volatile ("outw %%ax, %%dx"::"a"(data), "d"(port))
#define out32(port, data)					asm volatile ("outl %%eax, %%dx"::"a"(data), "d"(port))

#define in8(port) ({										\
	uint8_t _data;											\
	asm volatile ("inb %%dx, %%al":"=a"(_data):"d"(port));	\
	_data; })
#define in16(port) ({										\
	uint16_t _data;											\
	asm volatile ("inw %%dx, %%ax":"=a"(_data):"d"(port));	\
	_data; })
#define in32(port) ({										\
	uint32_t _data;											\
	asm volatile ("inl %%dx, %%eax":"=a"(_data):"d"(port));	\
	_data; })

#define store_eflags(eflags) 				asm volatile ("push %0\npopfl"::"r"((uint32_t) (eflags)):"cc", "memory")

#define load_eflags() ({									\
	uint32_t _eflags;										\
	asm volatile ("pushfl\npopl %0"):"=r"(_eflags);			\
	_eflags; })

#define store_cr0(cr0)						asm volatile ("mov %0, %%cr0"::"r"(cr0):"memory")

#define load_cr0() ({										\
	uint32_t _cr0;											\
	asm volatile ("mov %%cr0, %0":"=r"(_cr0));				\
	_cr0; })

#define load_tr(sel)						asm volatile ("ltr %0"::"r"(sel))

#ifdef __cplusplus
	}
#endif

#endif
