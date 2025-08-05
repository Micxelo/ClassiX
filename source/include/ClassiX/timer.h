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
} timer_state_t;

/* 定时器回调函数类型 */
typedef void (*timer_callback_t)(void *arg);

typedef struct TIMER {
	uint32_t id;				/* 定时器 ID */
	uint64_t expire_tick;		/* 到期时的系统滴答数 */
	uint32_t interval;			/* 重复间隔，0 表示不重复 */
	timer_callback_t callback;	/* 回调函数 */
	void *arg;					/* 传递给回调函数的参数 */
	timer_state_t state;		/* 定时器状态 */
	struct TIMER *next;			/* 指向下一个定时器的指针 */
} TIMER;

typedef struct {
	TIMER *head;				/* 定时器链表的头指针 */
	uint32_t next_id;			/* 下一个可用的定时器 ID */
	uint64_t *current_tick;		/* 当前系统滴答数 */
} TIMER_MANAGER;

void timer_init(void);
uint32_t timer_create(uint32_t interval, timer_callback_t callback, void *arg);
int timer_start(uint32_t timer_id);
int timer_stop(uint32_t timer_id);
int timer_restart(uint32_t timer_id, uint64_t new_interval);
int timer_delete(uint32_t timer_id);
void timer_process(void);
void timer_cleanup(void);
uint32_t timer_get_count(void);
uint32_t timer_get_active_count(void);

#ifdef __cplusplus
	}
#endif

#endif
