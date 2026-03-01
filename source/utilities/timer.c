/*
	utilities/timer.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/pit.h>
#include <ClassiX/spinlock.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>

static TIMER *timer_head = NULL;						/* 定时器链表的头指针 */
static spinlock_t timer_lock = SPINLOCK_INITIALIZER;	/* 自旋锁，保护定时器链表 */

/*
	@brief 创建一个新的定时器。
	@param callback 定时器到期时调用的回调函数
	@param arg 传递给回调函数的参数
	@return 新定时器，失败返回 NULL
*/
TIMER *timer_create(TIMER_CALLBACK callback, void *arg)
{
	if (callback == NULL) {
		debug("TIMER: Invalid timer parameters.\n");
		return 0; /* 无效参数 */
	}

	TIMER *new_timer = (TIMER *) kmalloc(sizeof(TIMER));
	if (!new_timer) {
		debug("TIMER: Failed to allocate memory for new timer.\n");
		return NULL; /* 内存分配失败 */
	}

	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

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

	spinlock_release_irqrestore(&timer_lock, eflags);
	debug("TIMER: Created timer %p.\n", new_timer);
	return new_timer;
}

/*
	@brief 启动指定定时器。
	@param timer 定时器
	@param interval 定时器触发间隔
	@param repetition 定时器重复次数，-1 表示循环
	@return 成功返回 0，失败返回 -1
*/
int32_t timer_start(TIMER *timer, uint64_t interval, int32_t repetition)
{
	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

	TIMER *current = timer_head;
	while (current) {
		if (current == timer) {
			if (current->state == TIMER_ACTIVE) {
				debug("TIMER: Timer %p is already active.\n", timer);
				spinlock_release_irqrestore(&timer_lock, eflags);
				return -1; /* 定时器已激活 */
			}
			current->interval = interval;
			current->repetition = repetition;
			current->expire_tick = get_system_ticks() + interval;
			current->state = TIMER_ACTIVE;
			spinlock_release_irqrestore(&timer_lock, eflags);
			debug("TIMER: Started timer %p, interval %llu ticks, expires at tick %llu, repeats %d times.\n",
				current, current->interval, current->expire_tick, repetition);
			return 0; /* 成功启动定时器 */
		}
		current = current->next;
	}

	spinlock_release_irqrestore(&timer_lock, eflags);
	debug("TIMER: Timer %p not found.\n", timer);
	return -1; /* 未找到定时器 */
}

/*
	@brief 停止指定定时器。
	@param timer 定时器
	@return 成功返回 0，失败返回 -1
*/
int32_t timer_stop(TIMER *timer)
{
	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

	TIMER *current = timer_head;
	while (current) {
		if (current == timer) {
			if (current->state != TIMER_ACTIVE) {
				debug("TIMER: Timer %p is not active.\n", timer);
				spinlock_release_irqrestore(&timer_lock, eflags);
				return -1; /* 定时器未激活 */
			}
			current->state = TIMER_INACTIVE;
			spinlock_release_irqrestore(&timer_lock, eflags);
			debug("TIMER: Stopped timer %p.\n", timer);
			return 0; /* 成功停止定时器 */
		}
		current = current->next;
	}

	spinlock_release_irqrestore(&timer_lock, eflags);
	debug("TIMER: Timer %p not found.\n", timer);
	return -1; /* 未找到定时器 */
}

/*
	@brief 删除指定定时器。
	@param timer 定时器
	@return 成功返回 0，失败返回 -1
*/
int32_t timer_delete(TIMER *timer)
{
	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

	TIMER *current = timer_head;
	TIMER *prev = NULL;
	while (current) {
		if (current == timer) {
			if (prev)
				prev->next = current->next;
			else
				timer_head = current->next;
			kfree(current);
			spinlock_release_irqrestore(&timer_lock, eflags);
			debug("TIMER: Deleted timer %p.\n", timer);
			return 0; /* 成功删除定时器 */
		}
		prev = current;
		current = current->next;
	}

	spinlock_release_irqrestore(&timer_lock, eflags);
	debug("TIMER: Timer %p not found.\n", timer);
	return -1; /* 未找到定时器 */
}

/*
	@brief 处理定时器过程。
*/
void timer_process(void)
{
	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

	uint64_t current_tick = get_system_ticks();
	TIMER *current = timer_head;
	while (current) {
		if (current->state == TIMER_ACTIVE && current->expire_tick <= current_tick) {
			/* 定时器到期，调用回调函数 */
			current->state = TIMER_EXPIRED;
			debug("TIMER: Timer %p expired at tick %llu, invoking callback.\n", current, current_tick);

			/* 执行回调：暂时释放锁，允许其他操作 */
			TIMER_CALLBACK callback = current->callback;
			void *arg = current->arg;
			spinlock_release_irqrestore(&timer_lock, eflags);	/* 释放锁，恢复之前的中断状态 */
			if (callback)
				callback(arg);
			eflags = spinlock_acquire_irqsave(&timer_lock);		/* 重新获取锁，保存新的中断状态 */

			/* 处理重复定时器 */
			if (current->state == TIMER_EXPIRED) {
				if (current->repetition < 0 || current->repetition-- > 0) {
					current->expire_tick = current_tick + current->interval;
					current->state = TIMER_ACTIVE;
					debug("TIMER: Reactivated periodic timer %p to expire at tick %llu.\n", current, current->expire_tick);
				}else {
					current->state = TIMER_INACTIVE;
					debug("TIMER: Timer %p set to inactive.\n", current);
				}
			}
		}
		current = current->next;
	}

	spinlock_release_irqrestore(&timer_lock, eflags);

	static uint64_t last_cleanup_tick = 0;
	current_tick = get_system_ticks();
	if (current_tick - last_cleanup_tick >= 60 * pit_frequency) {
		/* 每 60 秒清理一次 */
		timer_cleanup();
		last_cleanup_tick = current_tick;
	}
}

/*
	@brief 清理未激活的定时器。
*/
void timer_cleanup(void)
{
	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

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
			debug("TIMER: Cleaned up inactive timer %p.\n", current);
			if (prev)
				current = prev->next;
			else
				current = timer_head;
		} else {
			prev = current;
			current = current->next;
		}
	}

	spinlock_release_irqrestore(&timer_lock, eflags);
	if (removed_count > 0)
		debug("TIMER: Cleaned up %d inactive timers.\n", removed_count);
}

/*
	@brief 获取当前定时器总数。
	@return 当前定时器总数
*/
uint32_t timer_get_count(void)
{
	uint32_t count = 0;
	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

	TIMER *current = timer_head;
	while (current) {
		count++;
		current = current->next;
	}

	spinlock_release_irqrestore(&timer_lock, eflags);
	return count;
}

/*
	@brief 获取当前激活的定时器数。
	@return 当前激活的定时器数
*/
uint32_t timer_get_active_count(void)
{
	uint32_t count = 0;
	uint32_t eflags = spinlock_acquire_irqsave(&timer_lock);

	TIMER *current = timer_head;
	while (current) {
		if (current->state == TIMER_ACTIVE)
			count++;
		current = current->next;
	}

	spinlock_release_irqrestore(&timer_lock, eflags);
	return count;
}