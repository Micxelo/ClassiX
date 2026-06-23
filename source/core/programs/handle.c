/*
	core/programs/handle.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/handle.h>
#include <ClassiX/memory.h>
#include <ClassiX/spinlock.h>
#include <ClassiX/typedef.h>

#include <string.h>

/*
	@brief 扩展句柄表容量。
	@param table 句柄表
	@return 0 成功，负值表示错误
	@note 该函数假设调用者已持有句柄表的锁。
*/
static int32_t handle_table_expand(HANDLE_TABLE *table)
{
	size_t new_capacity = (table->capacity == 0) ? 16 : table->capacity * 2;
	if (new_capacity > table->max_capacity)
		new_capacity = table->max_capacity;
	if (new_capacity <= table->capacity)
		return -1; /* 已达最大容量 */

	HANDLE_ENTRY *new_entries = krealloc(table->entries, new_capacity * sizeof(HANDLE_ENTRY));
	if (!new_entries)
		return -1;

	for (size_t i = table->capacity; i < new_capacity; i++) {
		new_entries[i].next_free = table->free_list_head;
		new_entries[i].generation = 0;
		new_entries[i].flags = 0;
		table->free_list_head = (int32_t) i;
	}

	table->entries = new_entries;
	table->capacity = new_capacity;
	return 0;
}

/*
	@brief 初始化句柄表。
	@param table 句柄表
	@param initial_capacity 初始容量
	@param max_capacity 最大容量
	@return 0 成功，负值表示错误
*/
int32_t handle_table_init(HANDLE_TABLE *table, uint32_t initial_capacity, uint32_t max_capacity)
{
	if (!table || initial_capacity > max_capacity || initial_capacity == 0 || max_capacity == 0)
		return -1;

	table->entries = kmalloc(initial_capacity * sizeof(HANDLE_ENTRY));
	if (!table->entries)
		return -1;

	for (size_t i = 0; i < initial_capacity; i++) {
		table->entries[i].next_free = (int32_t) i + 1;
		table->entries[i].flags = 0;
		table->entries[i].generation = 0;
	}
	table->entries[initial_capacity - 1].next_free = -1; /* 末尾 */
	table->free_list_head = 0;

	table->capacity = initial_capacity;
	table->max_capacity = max_capacity;
	spinlock_init(&table->lock);
	return 0;
}

/*
	@brief 销毁句柄表。
	@param table 句柄表
*/
void handle_table_destroy(HANDLE_TABLE *table)
{
	if (table && table->entries) {
		spinlock_acquire(&table->lock);
		kfree(table->entries);
		table->entries = NULL;
		table->capacity = 0;
		table->free_list_head = -1;
		spinlock_release(&table->lock);
	}
}

/*
	@brief 分配句柄。
	@param table 句柄表
	@param object 关联对象指针
	@param flags 句柄标志（仅低两位有效）
	@param destructor 对象析构函数
	@return 分配的句柄，失败时返回 `HANDLE_NULL`
*/
HANDLE handle_table_alloc(HANDLE_TABLE *table, void *object, uint32_t flags, void (*destructor)(void *))
{
	if (!table || !object)
		return HANDLE_NULL;

	HANDLE handle = HANDLE_NULL;

	spinlock_acquire(&table->lock);

	/* 无空闲槽位时尝试扩容 */
	if (table->free_list_head == -1)
		if (handle_table_expand(table) != 0) {
			spinlock_release(&table->lock);
			return HANDLE_NULL; /* 扩容失败 */
		}

	/* 从空闲链表头分配 */
	int32_t index = table->free_list_head;
	HANDLE_ENTRY *entry = &table->entries[index];
	table->free_list_head = entry->next_free; /* 更新空闲链表头 */

	/* 初始化分配的槽位 */
	entry->object = object;
	entry->flags = flags & 0b00000011; /* 仅保留低两位作为标志 */
	entry->generation = (entry->generation + 1) & 0xFF;
	if (entry->generation == 0)
		entry->generation = 1; /* 代数递增，保持在 1-255 */

	handle.flags = entry->flags;
	handle.generation = entry->generation;
	handle.index = (uint32_t) index;

	spinlock_release(&table->lock);
	return handle;
}

/*
	@brief 释放句柄。
	@param table 句柄表
	@param handle 待释放的句柄
*/
void handle_table_free(HANDLE_TABLE *table, HANDLE handle)
{
	if (!table || handle.value == 0)
		return;

	spinlock_acquire(&table->lock);

	if (handle.index >= table->capacity ||
		table->entries[handle.index].object == NULL ||
		table->entries[handle.index].generation != handle.generation ||
		table->entries[handle.index].flags != handle.flags) {
		/* 无效句柄 */
		debug("HANDLE: Invalid handle 0x%08x\n", handle.value);
		spinlock_release(&table->lock);
		return;
	}

	table->entries[handle.index].next_free = table->free_list_head; /* 加入空闲链表 */
	table->free_list_head = (int32_t) handle.index;
	table->entries[handle.index].object = NULL;

	if (table->entries[handle.index].destructor)
		table->entries[handle.index].destructor(table->entries[handle.index].object);

	spinlock_release(&table->lock);		
}

/*
	@brief 查找句柄对应的对象指针。
	@param table 句柄表
	@param handle 待查找的句柄
	@return 关联对象指针，失败时返回 `NULL`
*/
void *handle_table_lookup(HANDLE_TABLE *table, HANDLE handle)
{
	if (!table || handle.value == 0)
		return NULL;

	spinlock_acquire(&table->lock);

	if (handle.index >= table->capacity ||
		table->entries[handle.index].object == NULL ||
		table->entries[handle.index].generation != handle.generation ||
		table->entries[handle.index].flags != handle.flags) {
		/* 无效句柄 */
		debug("HANDLE: Invalid handle 0x%08x\n", handle.value);
		spinlock_release(&table->lock);
		return NULL;
	}

	void *object = table->entries[handle.index].object;
	spinlock_release(&table->lock);
	return object;
}
