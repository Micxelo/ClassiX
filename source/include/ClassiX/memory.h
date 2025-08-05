/*
	include/ClassiX/memory.h
*/

#ifndef _CLASSIX_MEMORY_H_
#define _CLASSIX_MEMORY_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define ALLOC_ALIGNMENT					(512)
#define ALIGN_UP(x, align)				(((x) + (align) - 1) & ~((align) - 1))

void memory_init(void *base, size_t size);
void *kmalloc(size_t size, HANDLE task);
void kfree(void *ptr);
size_t get_total_memory(void);
size_t get_free_memory(void);

#ifdef __cplusplus
	}
#endif

#endif
