/*
	include/ClassiX/cpu.h
*/

#ifndef _CLASSIX_CPU_H_
#define _CLASSIX_CPU_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef struct {
	uint32_t level;
	size_t size;
	size_t line_size;
} cache_info_t;

bool check_cpuid_support(void);
bool check_tsc_support(void);
bool check_tsc_invariant(void);
uint64_t rdtsc(void);
void get_cpu_vendor(char *buf);
void get_cpu_brand(char *buf);
uint32_t get_apic_id();
int32_t get_cache_count();
int32_t get_cache_info(int32_t index, cache_info_t *buf);

#ifdef __cplusplus
	}
#endif

#endif