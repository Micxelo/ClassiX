/*
	core/memory.c
*/

#include <ClassiX/memory.h>
#include <ClassiX/typedef.h>

#define BLOCK_FREE							0
#define BLOCK_USED							1
#define BLOCK_MAGIC							0x0d000721

typedef struct __attribute__((aligned(16))) {
	uint32_t magic;
	size_t size;		/* 包括头尾 */
	uint8_t state;
	HANDLE task;
} block_header_t;

typedef struct __attribute__((aligned(16))) {
	size_t size;		/* 必须与 header 中 size 一致 */
} block_footer_t;

typedef struct freeblock_t {
	struct freeblock_t *prev;
	struct freeblock_t *next;
} freeblock_t;

#define MIN_BLOCK_SIZE						(sizeof(block_header_t) + sizeof(block_footer_t) + sizeof(freeblock_t))

static freeblock_t *freeblock_head = NULL;
static void *memory_pool_start;
static size_t memory_pool_size;
static void *memory_pool_end;

/*
	@brief 初始化内存池。
	@param base 内存池起始地址
	@param size 内存池大小
*/
void memory_init(void *base, size_t size)
{
	uintptr_t aligned_base = ALIGN_UP((uintptr_t) base, ALLOC_ALIGNMENT);
	size_t aligned_size = size - (aligned_base - (uintptr_t) base);

	memory_pool_start = (void*) aligned_base;
	memory_pool_size = aligned_size;
	memory_pool_end = (void*) ((uint8_t*) aligned_base + aligned_size);

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
	freeblock_head = freeblock;
}

/*
	@brief 分配内存。
	@param size 需要分配的字节数
	@param task 任务句柄
	@return 分配的内存指针，失败返回 NULL
*/
void *kmalloc(size_t size, HANDLE task)
{
	if (size == 0) return NULL;

	size_t total_size = size + sizeof(block_header_t) + sizeof(block_footer_t);

	/* 对齐到 ALLOC_ALIGNMENT */
	total_size = ALIGN_UP(total_size, ALLOC_ALIGNMENT);

	/* 遍历空闲列表 */
	freeblock_t *current = freeblock_head;
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
				if (freeblock_head == current) freeblock_head = current->next;
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
				if (freeblock_head == current) freeblock_head = new_free;
			}

			header->state = BLOCK_USED;
			header->task = task;
			return (void *) ((uint8_t *) header + sizeof(block_header_t));
		}
		current = current->next;
	}
	/* 内存不足 */
	return NULL;
}

/*
	@brief 释放内存。
	@param ptr 待释放的内存指针
*/
void kfree(void *ptr)
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
	if ((uint8_t *) next_header + sizeof(block_header_t) <= (uint8_t *) memory_pool_end 
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
		if (freeblock_head == next_free) freeblock_head = next_free->next;
	}

	/* 向前合并 */
	if ((uint8_t *) header > (uint8_t *) memory_pool_start) {
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
	new_free->next = freeblock_head;
	if (freeblock_head) freeblock_head->prev = new_free;
	freeblock_head = new_free;
	return;	
}

/*
	@brief 获取总内存大小。
	@return 总内存大小（字节）
*/
size_t get_total_memory(void)
{
	return memory_pool_size;
}

/*
	@brief 获取空闲内存大小。
	@return 空闲内存大小（字节）
*/
size_t get_free_memory(void)
{
	size_t total_free = 0;
	freeblock_t *current = freeblock_head;

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
