/*
	utilities/timer.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/pit.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>

static TIMER_MANAGER timer_manager;		/* 定时器管理器 */
static volatile int timer_lock = 0;		/* 互斥锁 */

/* 获取锁 */
static void timer_lock_acquire(void)
{
	while (__sync_lock_test_and_set(&timer_lock, 1))
		pause(); /* 自旋等待 */
}

/* 释放锁 */
static void timer_lock_release(void)
{
	__sync_lock_release(&timer_lock);
}

/*
	@brief 初始化定时器管理器。
*/
void timer_init(void)
{
	timer_manager.head = NULL;
	timer_manager.next_id = 1; /* 从 1 开始分配 ID */

	extern uint64_t system_ticks;
	timer_manager.current_tick = &system_ticks;

	debug("Timer manager initialized.\n");
}

/*
	@brief 创建一个新的定时器。
	@param interval 定时器间隔（以系统滴答为单位）
	@param callback 定时器到期时调用的回调函数
	@param arg 传递给回调函数的参数
	@return 新定时器的 ID，失败返回 0
	@note 此函数请求内存分配。
*/
uint32_t timer_create(uint32_t interval, timer_callback_t callback, void *arg)
{
	if (interval == 0 || callback == NULL) {
		debug("Invalid timer parameters.\n");
		return 0; /* 无效参数 */
	}
	
	TIMER *new_timer = (TIMER *) kmalloc(sizeof(TIMER), NULL);
	if (!new_timer) {
		debug("Failed to allocate memory for new timer.\n");
		return 0; /* 内存分配失败 */
	}

	timer_lock_acquire();
	
	new_timer->id = timer_manager.next_id++;
	new_timer->expire_tick = 0;		/* 尚未设置到期时间 */
	new_timer->interval = interval;
	new_timer->callback = callback;
	new_timer->arg = arg;
	new_timer->state = TIMER_INACTIVE;
	new_timer->next = NULL;

	/* 将新定时器插入链表 */
	if (!timer_manager.head) {
		timer_manager.head = new_timer;
	} else {
		TIMER *current = timer_manager.head;
		while (current->next) {
			current = current->next;
		}
		current->next = new_timer;
	}
	
	timer_lock_release();
	debug("Created timer ID %d with interval %d ticks.\n", new_timer->id, interval);
	return new_timer->id;
}

/*
	@brief 启动指定 ID 的定时器。
	@param timer_id 定时器 ID
	@return 成功返回 0，失败返回 -1
*/
int timer_start(uint32_t timer_id)
{
	timer_lock_acquire();

	TIMER *current = timer_manager.head;
	while (current) {
		if (current->id == timer_id) {
			if (current->state == TIMER_ACTIVE) {
				debug("Timer ID %d is already active.\n", timer_id);
				timer_lock_release();
				return -1; /* 定时器已激活 */
			}
			current->expire_tick = *timer_manager.current_tick + current->interval;
			current->state = TIMER_ACTIVE;
			timer_lock_release();
			debug("Started timer ID %d, expires at tick %llu.\n", timer_id, current->expire_tick);
			return 0; /* 成功启动定时器 */
		}
		current = current->next;
	}

	timer_lock_release();
	debug("Timer ID %d not found.\n", timer_id);
	return -1; /* 未找到定时器 */
}

/*
	@brief 停止指定 ID 的定时器。
	@param timer_id 定时器 ID
	@return 成功返回 0，失败返回 -1
*/
int timer_stop(uint32_t timer_id)
{
	timer_lock_acquire();

	TIMER *current = timer_manager.head;
	while (current) {
		if (current->id == timer_id) {
			if (current->state != TIMER_ACTIVE) {
				debug("Timer ID %d is not active.\n", timer_id);
				timer_lock_release();
				return -1; /* 定时器未激活 */
			}
			current->state = TIMER_INACTIVE;
			timer_lock_release();
			debug("Stopped timer ID %d.\n", timer_id);
			return 0; /* 成功停止定时器 */
		}
		current = current->next;
	}

	timer_lock_release();
	debug("Timer ID %d not found.\n", timer_id);
	return -1; /* 未找到定时器 */
}

/*
	@brief 重启指定 ID 的定时器。
	@param timer_id 定时器 ID
	@param new_interval 新的定时器间隔（以系统滴答为单位），如果为 0 则保持原间隔
	@return 成功返回 0，失败返回 -1
*/
int timer_restart(uint32_t timer_id, uint64_t new_interval)
{
	timer_lock_acquire();

	TIMER *current = timer_manager.head;
	while (current) {
		if (current->id == timer_id) {
			if (new_interval > 0) {
				current->interval = new_interval;
			}
			current->expire_tick = *timer_manager.current_tick + current->interval;
			current->state = TIMER_ACTIVE;
			timer_lock_release();
			debug("Restarted timer ID %d with new interval %d ticks, expires at tick %llu.\n",
				timer_id, current->interval, current->expire_tick);
			return 0; /* 成功重启定时器 */
		}
		current = current->next;
	}

	timer_lock_release();
	debug("Timer ID %d not found.\n", timer_id);
	return -1; /* 未找到定时器 */
}

/*
	@brief 删除指定 ID 的定时器。
	@param timer_id 定时器 ID
	@return 成功返回 0，失败返回 -1
*/
int timer_delete(uint32_t timer_id)
{
	timer_lock_acquire();
	
	TIMER *current = timer_manager.head;
	TIMER *prev = NULL;
	while (current) {
		if (current->id == timer_id) {
			if (prev)
				prev->next = current->next;
			else
				timer_manager.head = current->next;
			kfree(current);
			timer_lock_release();
			debug("Deleted timer ID %d.\n", timer_id);
			return 0; /* 成功删除定时器 */
		}
		prev = current;
		current = current->next;
	}

	timer_lock_release();
	debug("Timer ID %d not found.\n", timer_id);
	return -1; /* 未找到定时器 */
}

/*
	@brief 处理所有到期的定时器。
*/
void timer_process(void)
{
	timer_lock_acquire();

	uint64_t current_tick = *timer_manager.current_tick;
	TIMER *current = timer_manager.head;
	while (current) {
		if (current->state == TIMER_ACTIVE && current->expire_tick <= current_tick) {
			/* 定时器到期，调用回调函数 */
			current->state = TIMER_EXPIRED;
			debug("Timer ID %d expired at tick %llu, invoking callback.\n", current->id, current_tick);
			
			/* 执行回调 */
			timer_callback_t callback = current->callback;
			void *arg = current->arg;
			timer_lock_release(); /* 释放锁以允许回调函数执行时其他操作 */
			if (callback)
				callback(arg);
			timer_lock_acquire(); /* 重新获取锁 */

			/* 处理重复定时器 */
			if (current->state == TIMER_EXPIRED) {
				if (current->interval > 0) {
					current->expire_tick = current_tick + current->interval;
					current->state = TIMER_ACTIVE;
					debug("Reactivated periodic timer ID %d to expire at tick %llu.\n", current->id, current->expire_tick);
				} else {
					current->state = TIMER_INACTIVE;
					debug("One-shot timer ID %d set to inactive after expiration.\n", current->id);
				}
			}
		}
		current = current->next;
	}

	timer_lock_release();

	static uint64_t last_cleanup_tick = 0;
	if (*timer_manager.current_tick - last_cleanup_tick >= 60000) {
		/* 每 60 秒清理一次 */
		timer_cleanup();
		last_cleanup_tick = *timer_manager.current_tick;
	}
}

/*
	@brief 清理未激活的定时器。
*/
void timer_cleanup(void)
{
	timer_lock_acquire();
	
	uint32_t removed_count = 0;
	TIMER *current = timer_manager.head;
	TIMER *prev = NULL;
	while (current) {	
		if (current->state == TIMER_INACTIVE) {
			if (prev)
				prev->next = current->next;
			else
				timer_manager.head = current->next;
			kfree(current);
			removed_count++;
			debug("Cleaned up inactive timer ID %d.\n", current->id);
			if (prev)
				current = prev->next;
			else
				current = timer_manager.head;
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

	TIMER *current = timer_manager.head;
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

	TIMER *current = timer_manager.head;
	while (current) {
		if (current->state == TIMER_ACTIVE)
			count++;
		current = current->next;
	}

	timer_lock_release();
	return count;
}
