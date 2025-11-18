/*
	devices/pit.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/pit.h>
#include <ClassiX/task.h>
#include <ClassiX/timer.h>
#include <ClassiX/typedef.h>

uint32_t pit_frequency;

static volatile uint64_t system_ticks = 0; /* 系统时钟滴答计数 */

/*
	@brief 初始化可编程间隔定时器（PIT）。
	@param frequency PIT 的工作频率（Hz）
*/
void init_pit(uint32_t frequency)
{
	/* 计算 PIT 的分频值 */
	uint16_t divisor = PIT_BASE_FREQ / frequency;
	pit_frequency = frequency;

	if (divisor == 0) {
		divisor = 1;		/* 最低分频值*/
		pit_frequency = PIT_BASE_FREQ;
		debug("PIT: PIT frequency too high, setting to max frequency.\n");
	}

	/* 配置 PIT 通道 0：模式 3（方波），先低字节后高字节 */
	out8(PIT_COMMAND, PIT_CMD_CH0 | PIT_CMD_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY);

	/* 写入分频值 */
	out8(PIT_CHANNEL0, divisor & 0xff);			/* 低字节 */
	out8(PIT_CHANNEL0, (divisor >> 8) & 0xff);	/* 高字节 */

	/* 注册 IRQ */
	extern void asm_isr_pit(void);
	idt_set_gate(INT_NUM_PIT, (uint32_t) asm_isr_pit, 0x08, AR_INTGATE32);
	debug("PIT: PIT initialized at %d Hz (divisor: %d).\n", frequency, divisor);
}

void isr_pit(ISR_PARAMS params)
{
	/* 增加系统时钟滴答计数 */
	system_ticks++;

	/* 定时器过程 */
	timer_process();

	/* 发送 EOI 到 PIC */
	out8(PIC0_OCW2, 0x20); /* 主 PIC EOI */

	/* 任务调度 */
	extern uint64_t next_schedule_tick;
	if (next_schedule_tick <= system_ticks)
		task_schedule();
}

/*
	@brief 获取当前系统时钟滴答计数。
	@return 当前系统时钟滴答计数
*/
inline uint64_t get_system_ticks(void)
{
	return system_ticks;
}

/*
	@brief 重置系统时钟滴答计数。
*/
void reset_system_ticks(void)
{
	system_ticks = 0;
}

/*
	@brief 阻塞延时。
	@param ms 延时时长（毫秒）
*/
inline void delay(uint32_t ms)
{
	uint64_t end_tick = ms * pit_frequency / 1000 + system_ticks;
	while (system_ticks < end_tick)
		hlt();
}
