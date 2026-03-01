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

#define out8(port, data)					asm volatile ("outb %%al, %%dx"::"a"((uint8_t) (data)), "d"(port))
#define out16(port, data)					asm volatile ("outw %%ax, %%dx"::"a"((uint16_t) (data)), "d"(port))
#define out32(port, data)					asm volatile ("outl %%eax, %%dx"::"a"((uint32_t) (data)), "d"(port))

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
	asm volatile ("pushfl\npopl %0":"=r"(_eflags));			\
	_eflags; })

typedef union {
	uint32_t value;
	struct {
		uint32_t pe : 1;	/* 保护模式启用 */
		uint32_t mp : 1;	/* 监控协处理器 */
		uint32_t em : 1;	/* x87 协处理器模拟 */
		uint32_t ts : 1;	/* 任务已切换 */
		uint32_t et : 1;	/* 扩展类型 */
		uint32_t ne : 1;	/* 数学错误中断 */
		uint32_t reserved0 : 10;
		uint32_t wp : 1;	/* 写保护 */
		uint32_t reserved1 : 1;
		uint32_t am : 1;	/* 对齐检查 */
		uint32_t reserved2 : 10;
		uint32_t nw : 1;	/* 非透写 */
		uint32_t cd : 1;	/* 缓存禁用 */
		uint32_t pg : 1;	/* 分页 */
	};
} CR0;

#define store_cr0(cr0)						asm volatile ("mov %0, %%cr0"::"r"(cr0):"memory")

#define load_cr0() ({										\
	uint32_t _cr0;											\
	asm volatile ("mov %%cr0, %0":"=r"(_cr0));				\
	_cr0; })

typedef union {
	uint32_t value;
	struct {
		uint32_t vme : 1;			/* 虚拟 8086 模式扩展 */
		uint32_t pvi : 1;			/* 保护模式虚拟中断 */
		uint32_t tsd : 1;			/* 时间戳禁用 */
		uint32_t de : 1;			/* 调试扩展 */
		uint32_t pse : 1;			/* 页大小扩展 */
		uint32_t pae : 1;			/* 物理地址扩展 */
		uint32_t mce : 1;			/* 机器检查异常 */
		uint32_t pge : 1;			/* 全局页 */
		uint32_t pce : 1;			/* 性能监视器计数器启用 */
		uint32_t osfxsr : 1;		/* OS 支持 FXSAVE/FXRSTOR */
		uint32_t osxmmexcpt : 1;	/* OS 支持处理未屏蔽的 SIMD 异常 */
		uint32_t umip : 1;			/* 用户指令防护 */
		uint32_t la57 : 1;			/* 57 位线性地址 */
		uint32_t vmxe : 1;			/* 虚拟机扩展启用 */
		uint32_t smxe : 1;			/* 安全模式扩展启用 */
		uint32_t reserved0 : 1;
		uint32_t fsgsbase : 1;		/* FSGSBASE 指令启用 */
		uint32_t pcide : 1;			/* PCID 启用 */
		uint32_t osxsave : 1;		/* OS 支持 XSAVE/XRSTOR */
		uint32_t reserved1 : 1;
		uint32_t smep : 1;			/* SMEP 启用 */
		uint32_t smap : 1;			/* SMAP 启用 */
		uint32_t pke : 1;			/* 保护密钥启用 */
		uint32_t cet : 1;			/* 控制流 */
		uint32_t pks : 1;			/* 为 SMP 启用保护密钥 */
		uint32_t reserved2 : 7;
	};
} CR4;

#define store_cr4(cr4)						asm volatile ("mov %0, %%cr4"::"r"(cr4):"memory")

#define load_cr4() ({										\
	uint32_t _cr4;											\
	asm volatile ("mov %%cr4, %0":"=r"(_cr4));				\
	_cr4; })

#define load_tr(sel)						asm volatile ("ltr %0"::"r"(sel))

#ifdef __cplusplus
	}
#endif

#endif
