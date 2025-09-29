/*
	core/task.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/fifo.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/pit.h>
#include <ClassiX/task.h>
#include <ClassiX/timer.h>

volatile uint64_t next_schedule_tick;			/* 下一次进行任务调度的系统滴答数 */

static uint32_t ticks_per_priority_unit = 10;	/* 每单位优先级对应的系统滴答数 */

static struct TASK_MANAGER {
	uint32_t running;		/* 正在运行的任务数量 */
	uint32_t now;			/* 当前任务 */
	TASK *tasks[MAX_TASKS];
	TASK tasks0[MAX_TASKS];
} *task_manager;

static void task_idle_entry(void)
{
	for(;;)
		hlt();
}

/*
	@brief 初始化多任务。
	@return 指向内核任务的指针
*/
TASK *init_multitasking(void)
{
	TASK *ktask, *idle;

	task_manager = kmalloc(sizeof(struct TASK_MANAGER));
	if (!task_manager) return NULL;

	for (int i = 0; i < MAX_TASKS; i++) {
		task_manager->tasks0[i].state = TASK_FREE;
		task_manager->tasks0[i].selector = (TASK_GDT0 + i) * 8;
		gdt_set_gate(TASK_GDT0 + i, (uint32_t) &task_manager->tasks0[i].tss, sizeof(TSS) - 1, AR_TSS32 & 0xff, AR_TSS32 >> 16);
	}

	ktask = task_alloc();
	ktask->state = TASK_RUNNING;
	ktask->priority = PRIORITY_HIGH;
	task_manager->running = 1;
	task_manager->now = 0;
	task_manager->tasks[0] = ktask;
	load_tr(ktask->selector);

	idle = task_alloc();
	idle->tss.esp = (uint32_t) kmalloc(DEFAULT_USER_STACK) + DEFAULT_USER_STACK;
	idle->tss.eip = (uint32_t) &task_idle_entry;
	idle->tss.es = 0x10;
	idle->tss.cs = 0x08;
	idle->tss.ss = 0x10;
	idle->tss.ds = 0x10;
	idle->tss.fs = 0x10;
	idle->tss.gs = 0x10;
	task_register(idle, PRIORITY_IDLE);

	ticks_per_priority_unit = (pit_frequency * TIME_SLICE_BASE_PER_PRIORITY_MS) / 1000;
	next_schedule_tick = get_system_ticks() + ktask->priority * ticks_per_priority_unit;

	debug("Multitasking initialized.\n");
	return ktask;
}

/*
	@brief 获取一个任务。
	@return 指向任务的指针
	@note 应自行设置入口点和栈指针。
*/
TASK *task_alloc(void)
{
	TASK *task;
	for (int i = 0; i < MAX_TASKS; i++) {
		if (task_manager->tasks0[i].state == TASK_FREE) {
			task = &task_manager->tasks0[i];
			task->state = TASK_USED;
			task->tss.eflags = 0x00000202;
			task->tss.eax = 0;
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			debug("Allocated task %p.\n", task);
			return task;
		}
	}
	
	debug("Failed to allocate free task.\n");
	return NULL; /* 无空闲任务 */
}

/*
	@brief 注册指定的任务。
	@param task 待注册的任务
	@param priority 任务优先级
	@note 亦可用于唤醒休眠的任务。
*/
void task_register(TASK *task, TASK_PRIORITY priority)
{
	task->priority = priority;
	if (task->state != TASK_RUNNING) {
		/* 避免重复注册任务 */
		task->state = TASK_RUNNING;
		task_manager->tasks[task_manager->running++] = task;
		if (priority > task_manager->tasks[task_manager->now]->priority) {
			/* 新任务优先级高于当前任务，立即切换 */
			for (int i = 0; i < (signed)task_manager->running; i++) {
				if (task_manager->tasks[i] == task) {
					task_manager->now = i;
					break;
				}
			}
			farjmp(0, task_manager->tasks[task_manager->now]->selector);
		}
	}
}

/*
	@brief 任务调度器。
*/
void task_schedule(void)
{
	TASK *task;
	task_manager->now++;
	if (task_manager->now >= task_manager->running)
		task_manager->now = 0;

	task = task_manager->tasks[task_manager->now];
	next_schedule_tick += task->priority * ticks_per_priority_unit;

	/* 跳转至对应任务 */
	if (task_manager->running >= 2)
		farjmp(0, task->selector);
}

/*
	@brief 使指定的任务休眠。
	@return 指向任务的指针
*/
void task_sleep(TASK *task)
{
	bool ts = false;

	if (task->state == TASK_RUNNING) {
		/* 指定的任务正在运行 */
		if (task == task_manager->tasks[task_manager->now])
			ts = true; /* 使自己休眠（Yield），需要进行任务切换 */

		for (int i = 0; i < (signed) task_manager->running; i++)
			if (task == task_manager->tasks[i]) {
				task_manager->running--;
				if (i < (signed) task_manager->now) task_manager->now--;
				task->state = TASK_USED;
				for (; i < (signed) task_manager->running; i++)
					task_manager->tasks[i] = task_manager->tasks[i + 1];
			}

		if (task_manager->now >= task_manager->running)
			task_manager->now = 0;
		if (ts)
			farjmp(0, task_manager->tasks[task_manager->now]->selector);
	}
}
