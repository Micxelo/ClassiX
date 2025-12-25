# FIFO - ClassiX 文档

> 当前位置: arch/utilities/fifo.md

## 概述

FIFO（先进先出）缓冲区提供一个线程安全的循环缓冲区实现，用于在任务间传递数据，并提供任务唤醒机制，当数据写入时自动唤醒等待中的任务。

## 数据结构

### `FIFO`

|字段|描述|类型|
|:-:|:-:|:-:|'
|`buf`|缓冲区指针|`uint32_t *`|
|`idx_read`|读取位置索引|`int32_t`|
|`idx_write`|写入位置索引|`int32_t`|
|`size`|缓冲区总大小|`size_t`|
|`free`|缓冲区空闲空间大小|`size_t`|
|`task`|关联的任务指针|`TASK *`|

## 接口

### `fifo_init`

初始化 FIFO 缓冲区。

**函数原型**

```c
void fifo_init(
	FIFO *fifo,
	size_t size,
	uint32_t *buf,
	TASK *task
);
```

|参数|描述|
|:-:|:-:|
|`fifo`|FIFO 结构体指针|
|`size`|缓冲区大小|
|`buf`|缓冲区指针|
|`task`|关联的任务指针（可设置为 `NULL`）|

### `fifo_push`

向 FIFO 缓冲区写入一个数据，并在写入后唤醒关联任务（如有）。

**函数原型**

```c
int32_t fifo_push(
	FIFO *fifo,
	uint32_t data
);
```

|参数|描述|
|:-:|:-:|
|`fifo`|FIFO 结构体指针|
|`data`|要写入的数据|

|返回值|描述|
|:-:|:-:|
|`0`|写入成功|
|`-1`|缓冲区已满，写入失败|

### `fifo_pop`

从 FIFO 缓冲区读取一个数据。

**函数原型**

```c
uint32_t fifo_pop(
	FIFO *fifo
);
```

|参数|描述|
|:-:|:-:|
|`fifo`|FIFO 结构体指针|

|返回值|描述|
|:-:|:-:|
|`uint32_t`|读取到的数据|
|`-1`|缓冲区为空，读取失败|

### `fifo_status`

获取 FIFO 缓冲区中当前的数据数量。

**函数原型**

```c
int32_t fifo_status(
	FIFO *fifo
);
```

|参数|描述|
|:-:|:-:|
|`fifo`|FIFO 结构体指针|

|返回值|描述|
|:-:|:-:|
|`int32_t`|缓冲区中的数据数量|
