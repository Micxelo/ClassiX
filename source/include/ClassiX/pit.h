/*
	include/ClassiX/pit.h
*/

#ifndef _CLASSIX_PIT_H_
#define _CLASSIX_PIT_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/interrupt.h>
#include <ClassiX/typedef.h>

/* PIT 端口 */
#define PIT_CHANNEL0						(0x40)
#define PIT_CHANNEL1						(0x41)
#define PIT_CHANNEL2						(0x42)
#define PIT_COMMAND							(0x43)

/* PIT 命令字 */
#define PIT_CMD_CH0							(0x00)		/* 通道 0 */
#define PIT_CMD_CH1							(0x40)		/* 通道 1 */
#define PIT_CMD_CH2							(0x80)		/* 通道 2 */
#define PIT_CMD_LATCH						(0x00)		/* 锁存计数器值 */
#define PIT_CMD_LOBYTE						(0x10)		/* 只读写低字节 */
#define PIT_CMD_HIBYTE						(0x20)		/* 只读写高字节 */
#define PIT_CMD_LOHI						(0x30)		/* 先低字节后高字节 */
#define PIT_CMD_MODE0						(0x00)		/* 中断结束计数 */
#define PIT_CMD_MODE1						(0x02)		/* 可编程单脉冲 */
#define PIT_CMD_MODE2						(0x04)		/* 速率发生器 */
#define PIT_CMD_MODE3						(0x06)		/* 方波发生器 */
#define PIT_CMD_MODE4						(0x08)		/* 软件触发选通 */
#define PIT_CMD_MODE5						(0x0a)		/* 硬件触发选通 */
#define PIT_CMD_BINARY						(0x00)		/* 二进制计数 */
#define PIT_CMD_BCD							(0x01)		/* BCD 计数 */

#define PIT_BASE_FREQ						(1193182)	/* PIT 的基准频率 (1.193182 MHz) */

extern uint32_t pit_frequency;

void init_pit(uint32_t frequency);
uint64_t get_system_ticks(void);
void reset_system_ticks(void);
void delay(uint32_t ms);

#ifdef __cplusplus
	}
#endif

#endif