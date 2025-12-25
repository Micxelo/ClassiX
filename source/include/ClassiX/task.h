/*
	include/ClassiX/task.h
*/

#ifndef _CLASSIX_TASK_H_
#define _CLASSIX_TASK_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>

#define MAX_TASKS							(1000)

#define DEFAULT_USER_STACK					(4 * 1024)
#define TIME_SLICE_BASE_PER_PRIORITY_MS		(5)

typedef enum {
	TASK_FREE = 0,
	TASK_USED,
	TASK_RUNNING
} TASK_STATE;

typedef enum {
	PRIORITY_IDLE = 1,
	PRIORITY_LOW,
	PRIORITY_NORMAL,
	PRIORITY_HIGH,
} TASK_PRIORITY;

typedef struct {
	uint32_t backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	uint32_t es, cs, ss, ds, fs, gs;
	uint32_t ldtr, iomap;
} TSS;

typedef struct TASK {
	int32_t selector;		/* GDT 选择子 */
	TASK_STATE state;		/* 任务状态 */
	TASK_PRIORITY priority;	/* 任务优先级 */
	FIFO fifo;
	TSS tss;
	struct {
		uint16_t limit_low;
		uint16_t base_low;
		uint8_t base_mid;
		uint8_t access;
		uint8_t granularity;
		uint8_t base_high;
	} ldt[2];				/* LDT 描述符 */
	uint32_t ds_base;		/* 数据段基址 */
} TASK;

TASK *init_multitasking(void);
TASK *task_alloc(void);
void task_register(TASK *task, TASK_PRIORITY priority);
void task_schedule(void);
void task_sleep(TASK *task);
TASK *task_get_current(void);

#ifdef __cplusplus
	}
#endif

#endif
