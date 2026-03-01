/*
	devices/buzzer.c
*/

#include <ClassiX/buzzer.h>
#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/pit.h>
#include <ClassiX/task.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>

#define SPEAKER_PORT						(0x61)		/* 扬声器控制端口 */
#define SPEAKER_GATE_BIT					(1 << 0)	/* 允许 PIT 计数器 2 输出 */
#define SPEAKER_DATA_BIT					(1 << 1)	/* 直接驱动扬声器 */
#define SPEAKER_ENABLE_BITS					(SPEAKER_GATE_BIT | SPEAKER_DATA_BIT)

/*
	@brief 设置 PIT 频率。
	@param divisor PIT 的分频值
*/
static void set_pit_frequency(uint16_t divisor)
{
	/* 配置 PIT 通道 2：模式 3（方波） */
	out8(PIT_COMMAND, PIT_CMD_CH2 | PIT_CMD_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY);

	out8(PIT_CHANNEL2, divisor & 0xFF);			/* 低字节 */
	out8(PIT_CHANNEL2, (divisor >> 8) & 0xFF);	/* 高字节 */
}

/*
	@brief 打开扬声器。
*/
static void speaker_on(void)
{
	uint8_t val = in8(SPEAKER_PORT);
	val |= SPEAKER_ENABLE_BITS;
	out8(SPEAKER_PORT, val);
}

/*
	@brief 关闭扬声器。
*/
static void speaker_off(void)
{
	uint8_t val = in8(SPEAKER_PORT);
	val &= ~SPEAKER_ENABLE_BITS;
	out8(SPEAKER_PORT, val);
}

/*
	@brief 发出蜂鸣声。
	@param freq 蜂鸣声频率（Hz）
	@param duration 蜂鸣声持续时间（毫秒）
*/
void beep(uint32_t freq, uint32_t duration)
{
	if (freq == 0) {
		speaker_off();
		return;
	}

	uint16_t divisor = PIT_BASE_FREQ / freq;
	if (divisor == 0) {
		divisor = 1; /* 最低分频值*/
		pit_frequency = PIT_BASE_FREQ;
		debug("BUZZER: PIT frequency too high, setting to max frequency.\n");
	}

	set_pit_frequency(divisor);
	speaker_on();

	delay(duration);

	speaker_off();
}

static TASK *async_buzzer_task = NULL;	/* 异步蜂鸣器任务 */
static volatile bool is_buzzer_active = false;	/* 蜂鸣器是否正在发声 */

/*
	@brief 异步蜂鸣器定时器回调函数。
*/
static void buzzer_timer_callback(void *arg)
{
	(void) arg;	/* 未使用参数 */

	speaker_off();
	is_buzzer_active = false;
}

/*
	@brief 异步蜂鸣器任务入口函数。
*/
static void __attribute__((noreturn)) async_buzzer_entry(void)
{
	TASK *task = task_get_current();

	for (;;) {
		cli();
		if (fifo_status(&task->fifo) < 2) {
			sti();
			task_sleep(task);
		} else {
			if (is_buzzer_active) {
				sti();
				continue; /* 如果蜂鸣器正在发声，忽略新的请求 */
			}

			uint32_t freq = fifo_pop(&task->fifo);
			uint32_t duration = fifo_pop(&task->fifo);
			sti();

			if (freq == 0)
				continue;
			
			TIMER *timer = timer_create(buzzer_timer_callback, NULL);
			if (!timer) {
				debug("BUZZER: Failed to create timer for async buzzer.\n");
				continue;
			}

			uint16_t divisor = PIT_BASE_FREQ / freq;
			if (divisor == 0) {
				divisor = 1; /* 最低分频值*/
				pit_frequency = PIT_BASE_FREQ;
				debug("BUZZER: PIT frequency too high, setting to max frequency.\n");
			}
			set_pit_frequency(divisor);
			is_buzzer_active = true;
			speaker_on();

			timer_start(timer, duration * pit_frequency / 1000, 0);
			/* 定时器会自动清理 */
		}
	}
}

/*
	@brief 初始化异步蜂鸣器。
*/
void async_buzzer_init(void)
{
	if (async_buzzer_task)
		return;

	async_buzzer_task = task_alloc();

	async_buzzer_task->tss.esp = (uint32_t) memory_alloc(&g_mp, DEFAULT_USER_STACK, async_buzzer_task) + DEFAULT_USER_STACK;
	async_buzzer_task->tss.eip = (uint32_t) &async_buzzer_entry;
	async_buzzer_task->tss.es = 0x10;
	async_buzzer_task->tss.cs = 0x08;
	async_buzzer_task->tss.ss = 0x10;
	async_buzzer_task->tss.ds = 0x10;
	async_buzzer_task->tss.fs = 0x10;
	async_buzzer_task->tss.gs = 0x10;

	task_register(async_buzzer_task, PRIORITY_NORMAL);
	fifo_init(&async_buzzer_task->fifo, 128, memory_alloc(&g_mp, 128 * sizeof(uint32_t), async_buzzer_task), async_buzzer_task);
}

/*
	@brief 异步发出蜂鸣声。
	@param freq 蜂鸣声频率（Hz）
	@param duration 蜂鸣声持续时间（毫秒）
*/
void async_beep(uint32_t freq, uint32_t duration)
{
	if (!async_buzzer_task) {
		debug("BUZZER: Async buzzer not initialized.\n");
		return;
	}

	cli();
	fifo_push(&async_buzzer_task->fifo, freq);
	fifo_push(&async_buzzer_task->fifo, duration);
	sti();
}
