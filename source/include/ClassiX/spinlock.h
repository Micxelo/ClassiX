/*
	include/ClassiX/spinlock.h
*/

#ifndef _CLASSIX_SPINLOCK_H_
#define _CLASSIX_SPINLOCK_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

/* 自旋锁 */
typedef struct {
	volatile int32_t locked; /* 锁状态：0 = 未锁定，1 = 已锁定 */
} spinlock_t;

#define SPINLOCK_INITIALIZER 				{ 0 } /* 自旋锁初始化器 */

/* 初始化自旋锁 */
static inline void spinlock_init(spinlock_t *lock)
{
	lock->locked = 0;
}

/* 获取自旋锁 */
static inline void spinlock_acquire(spinlock_t *lock)
{
	while (__sync_lock_test_and_set(&lock->locked, 1))
		pause(); /* 自旋等待 */
	__sync_synchronize();
}

/* 释放自旋锁 */
static inline void spinlock_release(spinlock_t *lock)
{
	__sync_synchronize();
	__sync_lock_release(&lock->locked);
}

/* 获取自旋锁并保存中断状态 */
static inline uint32_t spinlock_acquire_irqsave(spinlock_t *lock)
{
	uint32_t eflags = load_eflags();
	cli();
	spinlock_acquire(lock);
	return eflags;
}

/* 释放自旋锁并恢复中断状态 */
static inline void spinlock_release_irqrestore(spinlock_t *lock, uint32_t eflags)
{
	spinlock_release(lock);
	store_eflags(eflags);
}

#ifdef __cplusplus
	}
#endif

#endif
