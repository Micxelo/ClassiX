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

#define DEFAULT_USER_STACK					(64 * 1024)
#define TIME_SLICE_BASE_PER_PRIORITY_MS		(1)

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

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} SEGMENT_DESCRIPTOR;

typedef struct TASK {
	/* 任务控制块 */
	int32_t selector;			/* GDT 选择子 */
	TASK_STATE state;			/* 任务状态 */
	TASK_PRIORITY priority;		/* 任务优先级 */
	FIFO fifo;					/* 任务专用 FIFO */
	TSS tss;					/* 任务状态段 */

	/* 应用程序用参数 */
	SEGMENT_DESCRIPTOR ldt[2];	/* 段描述符 */
	uint32_t code_base;			/* 代码段基址 */
	uint32_t code_limit;		/* 代码段界限 */
	uint32_t data_base;			/* 数据段基址 */
	uint32_t data_limit;		/* 数据段界限 */
	const char *path;			/* 应用程序路径 */
	int32_t argc;				/* 参数数量 */
	char **argv;				/* 参数数组 */

	/* FPU 数据 */
	bool fpu_used; /* 是否使用过 FPU */
	uint8_t fpu_state[512] __attribute__((aligned(16))); /* FPU 数据 */
} TASK;

#define TID(task)							(((task)->selector) / 8)

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
