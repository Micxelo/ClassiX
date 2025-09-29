/*
	include/ClassiX/memory.h
*/

#ifndef _CLASSIX_MEMORY_H_
#define _CLASSIX_MEMORY_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define ALLOC_ALIGNMENT					(128)
#define ALIGN_UP(x, align)				(((x) + (align) - 1) & ~((align) - 1))

typedef struct {
	void *start;
	size_t size;
	void *head;		/* 空闲块列表头 */
} MEMORY_POOL;

extern MEMORY_POOL g_mp;

void memory_init(MEMORY_POOL *pool, void *base, size_t size);
void *memory_alloc(MEMORY_POOL *pool, size_t size);
void memory_free(MEMORY_POOL *pool, void *ptr);

void *kmalloc(size_t size);
void *krealloc(void *ptr, size_t new_size);
void *kcalloc(size_t nmemb, size_t size);
void kfree(void *ptr);

size_t get_free_memory(const MEMORY_POOL *pool);

#ifdef __cplusplus
	}
#endif

#endif
