/*
	utilities/timer.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/pit.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>

static TIMER *timer_head = NULL;		/* 定时器链表的头指针 */
static volatile int timer_lock = 0;		/* 互斥锁 */

/* 获取锁 */
static void timer_lock_acquire(void)
{
	uint32_t eflags = load_eflags();
	cli();
	while (__sync_lock_test_and_set(&timer_lock, 1))
		pause(); /* 自旋等待 */
	store_eflags(eflags);
}

/* 释放锁 */
static void timer_lock_release(void)
{
	uint32_t eflags = load_eflags();
	cli();
	__sync_lock_release(&timer_lock);
	store_eflags(eflags);
}

/*
	@brief 创建一个新的定时器。
	@param callback 定时器到期时调用的回调函数
	@param arg 传递给回调函数的参数
	@return 新定时器，失败返回 NULL
*/
TIMER *timer_create(TIMER_CALLBACK callback, void *arg)
{
	if (callback == NULL) {
		debug("Invalid timer parameters.\n");
		return 0; /* 无效参数 */
	}
	
	TIMER *new_timer = (TIMER *) kmalloc(sizeof(TIMER));
	if (!new_timer) {
		debug("Failed to allocate memory for new timer.\n");
		return NULL; /* 内存分配失败 */
	}

	timer_lock_acquire();
	
	new_timer->expire_tick = 0;		/* 尚未设置到期时间 */
	new_timer->callback = callback;
	new_timer->arg = arg;
	new_timer->state = TIMER_INACTIVE;
	new_timer->next = NULL;

	/* 将新定时器插入链表 */
	if (!timer_head) {
		timer_head = new_timer;
	} else {
		TIMER *current = timer_head;
		while (current->next) {
			current = current->next;
		}
		current->next = new_timer;
	}
	
	timer_lock_release();
	debug("Created timer %p.\n", new_timer);
	return new_timer;
}

/*
	@brief 启动指定定时器。
	@param timer 定时器
	@param interval 定时器触发间隔
	@param repetition 定时器重复次数，-1 表示循环
	@return 成功返回 0，失败返回 -1
*/
int timer_start(TIMER *timer, uint64_t interval, int32_t repetition)
{
	timer_lock_acquire();

	TIMER *current = timer_head;
	while (current) {
		if (current == timer) {
			if (current->state == TIMER_ACTIVE) {
				debug("Timer %p is already active.\n", timer);
				timer_lock_release();
				return -1; /* 定时器已激活 */
			}
			current->interval = interval;
			current->repetition = repetition;
			current->expire_tick = system_ticks + interval;
			current->state = TIMER_ACTIVE;
			timer_lock_release();
			debug("Started timer %p, interval %d ticks, expires at tick %llu, repeats %d times.\n",
				current, current->interval, current->expire_tick, repetition);
			return 0; /* 成功启动定时器 */
		}
		current = current->next;
	}

	timer_lock_release();
	debug("Timer %p not found.\n", timer);
	return -1; /* 未找到定时器 */
}

/*
	@brief 停止指定定时器。
	@param timer 定时器
	@return 成功返回 0，失败返回 -1
*/
int timer_stop(TIMER *timer)
{
	timer_lock_acquire();

	TIMER *current = timer_head;
	while (current) {
		if (current == timer) {
			if (current->state != TIMER_ACTIVE) {
				debug("Timer %p is not active.\n", timer);
				timer_lock_release();
				return -1; /* 定时器未激活 */
			}
			current->state = TIMER_INACTIVE;
			timer_lock_release();
			debug("Stopped timer %p.\n", timer);
			return 0; /* 成功停止定时器 */
		}
		current = current->next;
	}

	timer_lock_release();
	debug("Timer %p not found.\n", timer);
	return -1; /* 未找到定时器 */
}

/*
	@brief 删除指定定时器。
	@param timer 定时器
	@return 成功返回 0，失败返回 -1
*/
int timer_delete(TIMER *timer)
{
	timer_lock_acquire();
	
	TIMER *current = timer_head;
	TIMER *prev = NULL;
	while (current) {
		if (current == timer) {
			if (prev)
				prev->next = current->next;
			else
				timer_head = current->next;
			kfree(current);
			timer_lock_release();
			debug("Deleted timer %p.\n", timer);
			return 0; /* 成功删除定时器 */
		}
		prev = current;
		current = current->next;
	}

	timer_lock_release();
	debug("Timer %p not found.\n", timer);
	return -1; /* 未找到定时器 */
}

/*
	@brief 处理定时器过程。
*/
void timer_process(void)
{
	timer_lock_acquire();

	uint64_t current_tick = system_ticks;
	TIMER *current = timer_head;
	while (current) {
		if (current->state == TIMER_ACTIVE && current->expire_tick <= current_tick) {
			/* 定时器到期，调用回调函数 */
			current->state = TIMER_EXPIRED;
			debug("Timer %p expired at tick %llu, invoking callback.\n", current, current_tick);
			
			/* 执行回调 */
			TIMER_CALLBACK callback = current->callback;
			void *arg = current->arg;
			timer_lock_release(); /* 释放锁以允许回调函数执行时其他操作 */
			if (callback)
				callback(arg);
			timer_lock_acquire(); /* 重新获取锁 */

			/* 处理重复定时器 */
			if (current->state == TIMER_EXPIRED) {
				if (current->repetition < 0 || current->repetition-- > 0) {
					current->expire_tick = current_tick + current->interval;
					current->state = TIMER_ACTIVE;
					debug("Reactivated periodic timer %p to expire at tick %llu.\n", current, current->expire_tick);
				}else {
					current->state = TIMER_INACTIVE;
					debug("Timer %p set to inactive.\n", current);
				}
			}
		}
		current = current->next;
	}

	timer_lock_release();

	static uint64_t last_cleanup_tick = 0;
	if (system_ticks - last_cleanup_tick >= 60 * pit_frequency) {
		/* 每 60 秒清理一次 */
		timer_cleanup();
		last_cleanup_tick = system_ticks;
	}
}

/*
	@brief 清理未激活的定时器。
*/
void timer_cleanup(void)
{
	timer_lock_acquire();
	
	uint32_t removed_count = 0;
	TIMER *current = timer_head;
	TIMER *prev = NULL;
	while (current) {	
		if (current->state == TIMER_INACTIVE) {
			if (prev)
				prev->next = current->next;
			else
				timer_head = current->next;
			kfree(current);
			removed_count++;
			debug("Cleaned up inactive timer %p.\n", current);
			if (prev)
				current = prev->next;
			else
				current = timer_head;
		} else {
			prev = current;
			current = current->next;
		}
	}

	timer_lock_release();
	if (removed_count > 0)
		debug("Cleaned up %d inactive timers.\n", removed_count);
}

/*
	@brief 获取当前定时器总数。
	@return 当前定时器总数
*/
uint32_t timer_get_count(void)
{
	uint32_t count = 0;
	timer_lock_acquire();

	TIMER *current = timer_head;
	while (current) {
		count++;
		current = current->next;
	}

	timer_lock_release();
	return count;
}

/*
	@brief 获取当前激活的定时器数。
	@return 当前激活的定时器数
*/
uint32_t timer_get_active_count(void)
{
	uint32_t count = 0;
	timer_lock_acquire();

	TIMER *current = timer_head;
	while (current) {
		if (current->state == TIMER_ACTIVE)
			count++;
		current = current->next;
	}

	timer_lock_release();
	return count;
}
