# 定时器 - ClassiX 文档

> 当前位置: arch/utilities/timer.md

## 概述

定时器子系统提供了一种基于系统滴答（tick）的定时器服务，支持单次触发、重复触发和可配置间隔的定时任务。该系统使用链表管理多个定时器，通过互斥锁保证线程安全，并提供了完整的生命周期管理功能。

### 实现方式

1. **链表管理**：所有定时器通过单向链表进行组织，支持动态创建和删除。
2. **锁机制**：使用自旋锁（`timer_lock`）保护对定时器链表的并发访问，确保在中断上下文或多任务环境下的数据一致性。
3. **基于系统滴答**：定时器的计时基于 PIT（可编程间隔定时器）提供的系统滴答计数，通过 `get_system_ticks()` 获取当前时间。
4. **状态机设计**：每个定时器具有三种状态：
   - `TIMER_INACTIVE`：未激活
   - `TIMER_ACTIVE`：已激活并正在计时
   - `TIMER_EXPIRED`：已到期，等待回调执行或重新调度
5. **重复与单次触发**：支持有限次数重复（通过 `repetition` 参数）或无限循环（`repetition = -1`）。
6. **自动清理**：系统每 60 秒自动清理未激活的定时器。

## 数据结构

### `TIMER`
定时器结构体，包含以下字段：
- `expire_tick`：到期时的系统滴答值
- `interval`：触发间隔（滴答数）
- `repetition`：重复次数（-1 表示无限循环）
- `callback`：到期回调函数指针
- `arg`：回调函数参数
- `state`：定时器状态（`TIMER_INACTIVE` / `TIMER_ACTIVE` / `TIMER_EXPIRED`）
- `next`：指向下一个定时器的指针

## 接口

### `timer_create`

创建一个新的定时器。

**函数原型**

```c
TIMER *timer_create(
	TIMER_CALLBACK callback,
	void *arg
);
```

|参数|描述|
|:-:|:-:|
|`callback`|定时器到期时调用的回调函数|
|`arg`|传递给回调函数的参数|

|返回值|描述|
|:-:|:-:|
|`TIMER *`|成功返回新创建的定时器指针，失败返回 `NULL`|

**说明**

- 回调函数类型为 `void (*)(void *)`
- 新创建的定时器初始状态为 `TIMER_INACTIVE`

### `timer_start`

启动指定定时器。

**函数原型**

```c
int32_t timer_start(
	TIMER *timer,
	uint64_t interval,
	int32_t repetition
);
```

|参数|描述|
|:-:|:-:|
|`timer`|要启动的定时器指针|
|`interval`|定时器触发间隔（滴答数）|
|`repetition`|重复次数（-1 表示无限循环）|

|返回值|描述|
|:-:|:-:|
|`int32_t`|成功返回 `0`，失败返回 `-1`|

**说明**

- 定时器到期时间设置为 `当前滴答 + interval`
- 如果定时器已处于激活状态，则启动失败

### `timer_stop`

停止指定定时器。

**函数原型**

```c
int32_t timer_stop(
	TIMER *timer
);
```

|参数|描述|
|:-:|:-:|
|`timer`|要停止的定时器指针|

|返回值|描述|
|:-:|:-:|
|`int32_t`|成功返回 `0`，失败返回 `-1`|

### `timer_delete`

删除指定定时器。

**函数原型**

```c
int32_t timer_delete(
	TIMER *timer
);
```

|参数|描述|
|:-:|:-:|
|`timer`|要删除的定时器指针|

|返回值|描述|
|:-:|:-:|
|`int32_t`|成功返回 `0`，失败返回 `-1`|

### `timer_process`

处理定时器到期事件。

**函数原型**

```c
void timer_process(void);
```

**说明**

- 应定期调用此函数（通常在每个系统滴答或时钟中断中）
- 触发到期的定时器回调函数
- 处理重复定时器的重新调度
- 每 60 秒自动调用一次 `timer_cleanup()`

### `timer_cleanup`

清理未激活的定时器。

**函数原型**

```c
void timer_cleanup(void);
```

### `timer_get_count`

获取当前定时器总数。

**函数原型**

```c
uint32_t timer_get_count(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前系统中存在的定时器总数|

### `timer_get_active_count`

获取当前激活的定时器数。

**函数原型**

```c
uint32_t timer_get_active_count(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前系统中处于激活状态的定时器数|
