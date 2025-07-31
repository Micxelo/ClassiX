/*
	devices/cpu.c
*/

#include <ClassiX/cpu.h>
#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

#include <string.h>

/*
	@brief 检查 CPU 是否支持 CPUID 指令。
	@return true - 支持 CPUID
*/
bool check_cpuid_support(void)
{
	uint32_t eflags_orig, eflags_mod;

	asm volatile(
		/* 保存原始 EFLAGS 值 */
		"pushfl\n"
		"popl %0\n"

		/* 复制 EFLAGS 并翻转 ID 位（bit 21） */
		"movl %0, %1\n"
		"xorl $0x200000, %1\n"

		/* 尝试写入修改后的EFLAGS */
		"pushl %1\n"
		"popfl\n"

		/* 读回 EFLAGS 检查是否修改成功 */
		"pushfl\n"
		"popl %1\n"

		/* 恢复原始EFLAGS */
		"pushl %0\n"
		"popfl\n"

		: "=&r"(eflags_orig), "=&r"(eflags_mod)
		:
		: "cc");

	/* 如果原始值和修改后值不同，即 ID 位可修改，支持 CPUID */
	return (eflags_orig ^ eflags_mod) & 0x200000;
}

/*
	@brief 检查 CPU 是否支持时间戳计数器 (TSC)。
	@return true - 支持 TSC
*/
bool check_tsc_support(void)
{
	uint32_t eax, ebx, ecx, edx;

	/* 使用 CPUID 功能 1 */
	asm volatile(
		"cpuid"
		:"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		:"a"(1));

	/* 检查 EDX 的第 4 位 (TSC 标志) */
	return (edx & (1 << 4)) != 0;
}

/*
	@brief 检查 CPU 是否支持 TSC 不变性。
	@return true - 支持 TSC 不变性
*/
bool check_tsc_invariant(void)
{
	uint32_t eax, ebx, ecx, edx;

	/* 首先检查是否支持扩展功能 0x80000007 */
	asm volatile(
		"cpuid"
		:"=a"(eax), "=d"(edx)
		:"a"(0x80000000)
		:"ebx", "ecx");

	/* 如果最大扩展功能号 < 0x80000007，则不支持检查 */
	if (eax < 0x80000007)
		return false;

	/* 获取扩展功能 0x80000007 */
	asm volatile(
		"cpuid"
		:"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		:"a"(0x80000007));

	/* 检查 EDX 的第 8 位 (Invariant TSC) */
	return (edx & (1 << 8)) != 0;
}

/*
	@brief 读取时间戳计数器 (TSC)。
*/
inline uint64_t rdtsc(void)
{
	uint32_t lo, hi;
	asm volatile(
		"mfence\n\t" /* 内存屏障 */
		"lfence\n\t" /* 加载屏障 */
		"rdtsc"
		:"=a"(lo), "=d"(hi)::"memory");
	asm volatile("lfence":::"memory"); /* 防止后续指令乱序 */
	return ((uint64_t) hi << 32) | lo;
}

/*
	@brief 获取 CPU 制造商 ID。
	@param buf 存储制造商 ID 的缓冲区，长度至少为 13 字节
*/
void get_cpu_vendor(char *buf)
{
	uint32_t ebx, ecx, edx;
	asm volatile("cpuid":"=b"(ebx), "=c"(ecx), "=d"(edx):"a"(0));
	*(uint32_t *) (buf) = ebx;
	*(uint32_t *) (buf + 4) = edx;
	*(uint32_t *) (buf + 8) = ecx;
	buf[12] = '\0';
}

/*
	@brief 获取 CPU 品牌字符串。
	@param brand 存储品牌字符串的缓冲区，长度至少为 49 字节
*/
void get_cpu_brand(char *brand)
{
	uint32_t eax, ebx, ecx, edx;
	for (int i = 0; i < 3; i++) {
		asm volatile("cpuid"
					 :"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
					 :"a"(0x80000002 + i));
		memcpy(brand + i * 16, &eax, 16); /* 复制 48 字节字符串 */
	}
	brand[48] = '\0';
}

/*
	@brief 获取 CPU APIC ID。
	@return APIC ID
*/
uint32_t get_apic_id()
{
	uint32_t ebx;
	asm volatile("cpuid":"=b"(ebx):"a"(1):"ecx", "edx");
	return ebx >> 24;
}

/*
	@brief 获取 CPU 缓存数量。
	@return 缓存数量
*/
int32_t get_cache_count()
{
	uint32_t eax, ebx, ecx, edx;
	int32_t i = 0;
	
	for (i = 0; ; i++) {
		asm volatile("cpuid"
					 :"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
					 :"a"(4), "c"(i)); /* EAX = 4，ECX = 缓存索引 */

		uint8_t cache_type = eax & 0x1f;
		if (cache_type == 0)
			break; /* 无效缓存 */
	}

	return i;
}

/*
	@brief 获取指定索引的 CPU 缓存信息。
	@param index 缓存索引
	@param buf 存储缓存信息的缓冲区
	@return 0 - 成功，1 - 无效缓存
*/
int32_t get_cache_info(int32_t index, cache_info_t *buf)
{
	uint32_t eax, ebx, ecx, edx;
	
	asm volatile("cpuid"
					:"=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
					:"a"(4), "c"(index)); /* EAX = 4，ECX = 缓存索引 */

	uint8_t cache_type = eax & 0x1f;
	if (cache_type == 0)
		return 1; /* 无效缓存 */

	uint8_t cache_level = (eax >> 5) & 0x7;
	uint32_t line_size = (ebx & 0xfff) + 1;
	uint32_t ways = ((ebx >> 22) & 0x3ff) + 1;
	uint32_t sets = ecx + 1;
	uint32_t size = ways * line_size * sets;

	buf->level = cache_level;
	buf->size = size;
	buf->line_size = line_size;
	return 0;
}
