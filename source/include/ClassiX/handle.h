/*
	include/ClassiX/handle.h
*/

#ifndef _CLASSIX_HANDLE_H_
#define _CLASSIX_HANDLE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/spinlock.h>
#include <ClassiX/typedef.h>

typedef union {
	struct {
		uint32_t flags : 2;			/* 标志 */
		uint32_t generation : 8;	/* 代数 */
		uint32_t index : 22;		/* 索引 */
	};
	uint32_t value;
} HANDLE;

static_assert(sizeof(HANDLE) == sizeof(uint32_t), "`HANDLE` must be 32 bits");

#define HANDLE_NULL							((HANDLE) { .value = 0 }) /* 空句柄 */

typedef struct {
	union {
		void *object;				/* 占用时的对象指针 */
		int32_t next_free;			/* 空闲时的下一个空闲句柄索引，-1 表示末尾 */
	};
	uint32_t flags;					/* 占用时的标志 */
	uint32_t generation;			/* 占用时的代数 */
	void (*destructor)(void *);		/* 对象析构函数 */
} HANDLE_ENTRY;

typedef struct {
	HANDLE_ENTRY *entries;			/* 句柄表 */
	uint32_t capacity;				/* 当前容量 */
	uint32_t max_capacity;			/* 最大容量 */
	spinlock_t lock;				/* 保护句柄表的自旋锁 */
	int32_t free_list_head;			/* 空闲链表头索引，-1 表示无空闲 */
} HANDLE_TABLE;

int32_t handle_table_init(HANDLE_TABLE *table, uint32_t initial_capacity, uint32_t max_capacity);
void handle_table_destroy(HANDLE_TABLE *table);
HANDLE handle_table_alloc(HANDLE_TABLE *table, void *object, uint32_t flags, void (*destructor)(void *));
void handle_table_free(HANDLE_TABLE *table, HANDLE handle);
void *handle_table_lookup(HANDLE_TABLE *table, HANDLE handle);

#ifdef __cplusplus
	}
#endif

#endif
