/*
	core/memory.c
*/

#include <ClassiX/memory.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>

#include <string.h>

#define BLOCK_FREE							(0)				/* 空闲 */
#define BLOCK_USED							(1)				/* 已分配 */
#define BLOCK_MAGIC							(0x0d000721)	/* 内存块魔数 */

typedef struct __attribute__((aligned(16))) {
	uint32_t magic;
	size_t size;		/* 包括头尾 */
	uint8_t state;
	TASK *task;			/* 使用此内存块的任务 */
} block_header_t;

typedef struct __attribute__((aligned(16))) {
	size_t size;		/* 必须与 header 中 size 一致 */
} block_footer_t;

typedef struct freeblock_t {
	struct freeblock_t *prev;
	struct freeblock_t *next;
} freeblock_t;

#define MIN_BLOCK_SIZE						(sizeof(block_header_t) + sizeof(block_footer_t) + sizeof(freeblock_t))

MEMORY_POOL g_mp;		/* 内核内存池 */

/*
	@brief 初始化内存池。
	@param base 内存池起始地址
	@param size 内存池大小
*/
void memory_init(MEMORY_POOL *pool, void *base, size_t size)
{
	uintptr_t aligned_base = ALIGN_UP((uintptr_t) base, ALLOC_ALIGNMENT);
	size_t aligned_size = size - (aligned_base - (uintptr_t) base);

	pool->start = (void*) aligned_base;
	pool->size = aligned_size;

	/* 初始化第一个内存块 */
	block_header_t *header = (block_header_t *) base;
	header->magic = BLOCK_MAGIC;
	header->size = size;
	header->state = BLOCK_FREE;
	header->task = NULL;

	block_footer_t *footer = (block_footer_t *) ((uint8_t *) base + size - sizeof(block_footer_t));
	footer->size = size;

	/* 初始化空闲列表 */
	freeblock_t *freeblock = (freeblock_t *) ((uint8_t *) header + sizeof(block_header_t));
	freeblock->prev = NULL;
	freeblock->next = NULL;
	pool->head = freeblock;
}

/*
	@brief 分配内存。
	@param pool 待操作的内存池
	@param size 需要分配的字节数
	@return 分配的内存指针，失败返回 NULL
*/
void *memory_alloc(MEMORY_POOL *pool, size_t size)
{
	if (size == 0) return NULL;

	size_t total_size = size + sizeof(block_header_t) + sizeof(block_footer_t);

	/* 对齐到 ALLOC_ALIGNMENT */
	total_size = ALIGN_UP(total_size, ALLOC_ALIGNMENT);

	/* 遍历空闲列表 */
	freeblock_t *current = pool->head;
	while (current) {
		block_header_t *header = (block_header_t *) ((uint8_t *) current - sizeof(block_header_t));
		if (header->magic != BLOCK_MAGIC) return NULL;

		if (header->size >= total_size) {
			/* 足够大，整体分配或分割 */
			size_t remaining = header->size - total_size;

			if (remaining < ALIGN_UP(MIN_BLOCK_SIZE, ALLOC_ALIGNMENT)) {
				/* 整体分配 */
				/* 从空闲链表中移除当前块 */
				if (current->prev) current->prev->next = current->next;
				if (current->next) current->next->prev = current->prev;
				if (pool->head == current) pool->head = current->next;
			} else {
				/* 分割 */
				header->size = total_size;
				block_footer_t *footer = (block_footer_t *) ((uint8_t *) header + total_size - sizeof(block_footer_t));
				footer->size = total_size;

				/* 创建新空闲块 */
				block_header_t *new_header = (block_header_t *) ((uint8_t *) header + total_size);
				new_header->magic = BLOCK_MAGIC;
				new_header->size = remaining;
				new_header->state = BLOCK_FREE;

				block_footer_t *new_footer =
					(block_footer_t *) ((uint8_t *) new_header + remaining - sizeof(block_footer_t));
				new_footer->size = remaining;

				/* 将新块加入空闲列表 */
				freeblock_t *new_free = (freeblock_t *) ((uint8_t *) new_header + sizeof(block_header_t));
				new_free->prev = current->prev;
				new_free->next = current->next;
				if (current->prev) current->prev->next = new_free;
				if (current->next) current->next->prev = new_free;
				if (pool->head == current) pool->head = new_free;
			}

			header->state = BLOCK_USED;
			header->task = NULL;
			return (void *) ((uint8_t *) header + sizeof(block_header_t));
		}
		current = current->next;
	}
	/* 内存不足 */
	return NULL;
}

/*
	@brief 释放内存。
	@param pool 待操作的内存池
	@param ptr 待释放的内存指针
*/
void memory_free(MEMORY_POOL *pool, void *ptr)
{
	if (ptr == NULL) return;

	block_header_t *header = (block_header_t *) ((uint8_t *) ptr - sizeof(block_header_t));
	if (!(header->magic == BLOCK_MAGIC && header->state == BLOCK_USED)) return;

	/* 标记为释放 */
	header->state = BLOCK_FREE;
	header->task = NULL;

	/* 合并相邻空闲块 */

	/* 向后合并 */
	block_header_t *next_header = (block_header_t *) ((uint8_t *) header + header->size);
	if ((uint8_t *) next_header + sizeof(block_header_t) <= ((uint8_t *) pool->start + pool->size)
		&& next_header->magic == BLOCK_MAGIC
		&& next_header->state == BLOCK_FREE) {
		/* 合并 */
		header->size += next_header->size;
		block_footer_t *footer = (block_footer_t *) ((uint8_t *) header + header->size - sizeof(block_footer_t));
		footer->size = header->size;

		/* 从空闲列表中移除下一个块 */
		freeblock_t *next_free = (freeblock_t *) ((uint8_t *) next_header + sizeof(block_header_t));
		if (next_free->prev) next_free->prev->next = next_free->next;
		if (next_free->next) next_free->next->prev = next_free->prev;
		if (pool->head == next_free) pool->head = next_free->next;
	}

	/* 向前合并 */
	if ((uint8_t *) header > (uint8_t *) pool->start) {
		block_footer_t *prev_footer = (block_footer_t *) ((uint8_t *) header - sizeof(block_footer_t));
		block_header_t *prev_header = (block_header_t *) ((uint8_t *) header - prev_footer->size);

		if (prev_header->magic == BLOCK_MAGIC && prev_header->state == BLOCK_FREE) {
			/* 合并 */
			prev_header->size += header->size;
			block_footer_t *footer =
				(block_footer_t *) ((uint8_t *) prev_header + prev_header->size - sizeof(block_footer_t));
			footer->size = prev_header->size;

			/* 当前块已被合并，无需加入链表 */
			return;
		}
	}

	/* 将当前块加入空闲块列表 */
	freeblock_t *new_free = (freeblock_t *) ((uint8_t *) header + sizeof(block_header_t));
	new_free->prev = NULL;
	new_free->next = pool->head;
	if (pool->head) ((freeblock_t *) (pool->head))->prev = new_free;
	pool->head = new_free;
	return;
}

/*
	@brief 分配内存。
	@param size 需要分配的字节数
	@return 分配的内存指针，失败返回 NULL
*/
void *kmalloc(size_t size)
{
	return memory_alloc(&g_mp, size);
}

/*
	@brief 重新分配内存块。
	@param ptr 原内存指针
	@param new_size 新的字节数
	@return 新分配的内存指针，失败返回 NULL
*/
void *krealloc(void *ptr, size_t new_size)
{
	if (ptr == NULL) {
		/* 等价于 kmalloc */
		return kmalloc(new_size);
	}
	if (new_size == 0) {
		/* 等价于 kfree */
		kfree(ptr);
		return NULL;
	}

	block_header_t *header = (block_header_t *) ((uint8_t *) ptr - sizeof(block_header_t));
	if (!(header->magic == BLOCK_MAGIC && header->state == BLOCK_USED)) return NULL;

	size_t old_size = header->size - sizeof(block_header_t) - sizeof(block_footer_t);
	if (new_size <= old_size) {
		/* 新尺寸小于等于旧尺寸，直接返回原指针 */
		return ptr;
	}

	/* 分配新内存并拷贝旧数据 */
	void *new_ptr = kmalloc(new_size);
	if (new_ptr == NULL) return NULL;
	memcpy(new_ptr, ptr, old_size);
	kfree(ptr);
	return new_ptr;
}

/*
	@brief 分配并清零内存块。
	@param nmemb 元素数量
	@param size 每个元素的字节数
	@return 分配的内存指针，失败返回 NULL
*/
void *kcalloc(size_t nmemb, size_t size)
{
	size_t total_size = nmemb * size;
	void *ptr = kmalloc(total_size);
	if (ptr)
		memset(ptr, 0, total_size); /* 清零分配的内存 */
	return ptr;
}

/*
	@brief 释放内存。
	@param ptr 待释放的内存指针
*/
void kfree(void *ptr)
{
	memory_free(&g_mp, ptr);
}

/*
	@brief 获取空闲内存大小。
	@return 空闲内存大小（字节）
*/
size_t get_free_memory(const MEMORY_POOL *pool)
{
	size_t total_free = 0;
	freeblock_t *current = pool->head;

	while (current) {
		block_header_t *header = (block_header_t *) ((uint8_t *) current - sizeof(block_header_t));

		if (!(header->magic == BLOCK_MAGIC && header->state == BLOCK_FREE)) {
			current = current->next;
			continue;
		}

		total_free += header->size - (sizeof(block_header_t) + sizeof(block_footer_t));
		current = current->next;
	}

	return total_free;
}
