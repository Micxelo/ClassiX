/*
	include/ClassiX/timer.h
*/

#ifndef _CLASSIX_TIMER_H_
#define _CLASSIX_TIMER_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef enum {
	TIMER_INACTIVE = 0,			/* 定时器未激活 */
	TIMER_ACTIVE,				/* 定时器已激活 */
	TIMER_EXPIRED				/* 定时器已过期 */
} TIMER_STATE;

/* 定时器回调函数类型 */
typedef void (*TIMER_CALLBACK) (void *arg);

typedef struct TIMER {
	uint64_t interval;			/* 触发间隔 */
	uint64_t expire_tick;		/* 到期时的系统滴答数 */
	int32_t repetition;			/* 重复次数 */
	TIMER_CALLBACK callback;	/* 回调函数 */
	void *arg;					/* 传递给回调函数的参数 */
	TIMER_STATE state;			/* 定时器状态 */
	struct TIMER *next;			/* 指向下一个定时器的指针 */
} TIMER;

TIMER *timer_create(TIMER_CALLBACK callback, void *arg);
int timer_start(TIMER *timer, uint64_t interval, int32_t repetition);
int timer_stop(TIMER *timer);
int timer_delete(TIMER *timer);
void timer_process(void);
void timer_cleanup(void);
uint32_t timer_get_count(void);
uint32_t timer_get_active_count(void);

#ifdef __cplusplus
	}
#endif

#endif
