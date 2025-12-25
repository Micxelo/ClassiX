# 内存管理 - ClassiX 文档

> 当前位置: arch/core/memory.md

## 概述

内存管理子系统提供动态内存分配功能，采用边界标记法和空闲链表实现高效的内存分配与回收，支持内存块的合并与分割。

## 数据结构

### 内存池（`MEMORY_POOL`）

|字段|类型|描述|
|:-:|:-:|:-:|
|`start`|`void *`|内存池起始地址|
|`size`|`size_t`|内存池总大小|
|`head`|`freeblock_t *`|空闲链表头指针|

### 内存块头（`block_header_t`）

|字段|类型|对齐|描述|
|:-:|:-:|:-:|:-:|
|`magic`|`uint32_t`|16 字节|魔数 `0x0d000721`，用于验证|
|`size`|`size_t`|16 字节|内存块总大小（包含头尾）|
|`state`|`uint8_t`|16 字节|块状态：`BLOCK_FREE`/`BLOCK_USED`|
|`task`|`TASK *`|16 字节|使用此内存块的任务指针|

### 内存块尾（`block_footer_t`）

|字段|类型|对齐|描述|
|:-:|:-:|:-:|:-:|
|`size`|`size_t`|16 字节|必须与头部 `size` 字段一致|

### 空闲块（`freeblock_t`）

|字段|类型|描述|
|:-:|:-:|:-:|
|`prev`|`freeblock_t *`|前驱空闲块指针|
|`next`|`freeblock_t *`|后继空闲块指针|

## 常量定义

|常量|值|描述|
|:-:|:-:|:-:|
|`BLOCK_FREE`|`0`|内存块空闲状态|
|`BLOCK_USED`|`1`|内存块已使用状态|
|`BLOCK_MAGIC`|`0x0d000721`|内存块魔数标识|
|`MIN_BLOCK_SIZE`|`sizeof(block_header_t) + sizeof(block_footer_t) + sizeof(freeblock_t)`|最小内存块大小|
|`ALLOC_ALIGNMENT`|`16`|内存分配对齐字节数|

## 全局变量

|变量|类型|描述|
|:-:|:-:|:-:|
|`g_mp`|`MEMORY_POOL`|内核内存池实例|

## 内存池管理

### `memory_init`

初始化内存池。

**函数原型**

```c
void memory_init(
	MEMORY_POOL *pool,
	void *base,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`pool`|待初始化的内存池指针|
|`base`|内存池起始地址|
|`size`|内存池总大小|

### `memory_alloc`

从指定内存池分配内存。

**函数原型**

```c
void *memory_alloc(
	MEMORY_POOL *pool,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`pool`|待操作的内存池|
|`size`|需要分配的字节数|

|返回值|描述|
|:-:|:-:|
|`void *`|分配的内存指针，失败返回 `NULL`|

### `memory_free`

释放内存到指定内存池。

**函数原型**

```c
void memory_free(
	MEMORY_POOL *pool,
	void *ptr
);
```

|参数|描述|
|:-:|:-:|
|`pool`|待操作的内存池|
|`ptr`|待释放的内存指针|

## 内核内存分配

### `kmalloc`

分配内核内存。

**函数原型**

```c
void *kmalloc(
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`size`|需要分配的字节数|

|返回值|描述|
|:-:|:-:|
|`void *`|分配的内存指针，失败返回 `NULL`|

### `kfree`

释放内核内存。

**函数原型**

```c
void kfree(
	void *ptr
);
```

|参数|描述|
|:-:|:-:|
|`ptr`|待释放的内存指针|

### `krealloc`

重新分配内核内存。

**函数原型**

```c
void *krealloc(
	void *ptr,
	size_t new_size
);
```

|参数|描述|
|:-:|:-:|
|`ptr`|原内存指针|
|`new_size`|新的字节数|

|返回值|描述|
|:-:|:-:|
|`void *`|新分配的内存指针，失败返回 `NULL`|

### `kcalloc`

分配并清零内核内存。

**函数原型**

```c
void *kcalloc(
	size_t nmemb,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`nmemb`|元素数量|
|`size`|每个元素的字节数|

|返回值|描述|
|:-:|:-:|
|`void *`|分配并清零的内存指针，失败返回 `NULL`|

## 内存信息

### `get_free_memory`

获取内存池中空闲内存大小。

**函数原型**
```c
size_t get_free_memory(
	const MEMORY_POOL *pool
);
```

|参数|描述|
|:-:|:-:|
|`pool`|待查询的内存池|

|返回值|描述|
|:-:|:-:|
|`size_t`|空闲内存大小（字节）|

## 内存布局

### 已分配内存块

```
+----------------+
| block_header_t | 16 字节对齐
|----------------| <- 返回给用户的指针
|   user data    | size 字节
|----------------|
| block_footer_t | 16 字节对齐
+----------------+
```

### 空闲内存块

```
+----------------+ <- 空闲链表指针
| block_header_t | 16 字节对齐
|----------------|
|  freeblock_t   | 前后指针
|----------------|
|   free space   | 
|----------------|
| block_footer_t | 16 字节对齐
+----------------+
```
