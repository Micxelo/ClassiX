/*
	include/ClassiX/blkdev.h
*/

#ifndef _CLASSIX_BLKDEV_H_
#define _CLASSIX_BLKDEV_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

/* IDE 端口定义 */
#define IDE_PRIMARY_BASE					0x1F0
#define IDE_SECONDARY_BASE					0x170

/* IDE 寄存器偏移 */
#define IDE_REG_DATA						0x0
#define IDE_REG_ERROR						0x1
#define IDE_REG_FEATURES					0x1
#define IDE_REG_SECTOR_COUNT				0x2
#define IDE_REG_LBA_LOW						0x3
#define IDE_REG_LBA_MID						0x4
#define IDE_REG_LBA_HIGH					0x5
#define IDE_REG_DRIVE_SELECT				0x6
#define IDE_REG_STATUS						0x7
#define IDE_REG_COMMAND						0x7

/* IDE 状态寄存器位 */
#define IDE_STATUS_ERR						(1 << 0)	/* 错误 */
#define IDE_STATUS_DRQ						(1 << 3)	/* 数据请求就绪 */
#define IDE_STATUS_SRV						(1 << 4)	/* 服务请求 */
#define IDE_STATUS_DF						(1 << 5)	/* 设备故障 */
#define IDE_STATUS_RDY						(1 << 6)	/* 设备就绪 */
#define IDE_STATUS_BSY						(1 << 7)	/* 设备忙 */

/* IDE 命令 */
#define IDE_CMD_IDENTIFY					0xEC
#define IDE_CMD_READ_DMA					0xC8
#define IDE_CMD_WRITE_DMA					0xCA
#define IDE_CMD_READ_DMA_EXT				0x25
#define IDE_CMD_WRITE_DMA_EXT				0x35

/* IDE 驱动器选择 */
#define IDE_DRIVE_MASTER					0xA0
#define IDE_DRIVE_SLAVE						0xB0
#define IDE_DRIVE_LBA_MASK					0x0F

/* DMA Bus Master 寄存器偏移 */
#define BM_COMMAND_REG						0x0
#define BM_STATUS_REG						0x2
#define BM_PRDT_ADDR_REG					0x4

/* Bus Master 命令寄存器位 */
#define BM_CMD_START						(1 << 0)
#define BM_CMD_READ_DIRECTION				(1 << 3)

/* Bus Master 状态寄存器位 */
#define BM_STATUS_INTR						(1 << 2)	/* 中断状态 */
#define BM_STATUS_ERROR						(1 << 1)	/* 错误 */
#define BM_STATUS_ACTIVE					(1 << 0)	/* 激活 */

/* PRD 标志 */
#define PRD_EOT								(1 << 15)	/* 结束标记 */

#define IDE_SECTOR_SIZE						512			/* 扇区大小 */
#define IDE_IDENTIFY_BUFFER_SIZE			512			/* IDENTIFY 数据缓冲区大小 */

/* 物理区域描述符 (PRD) */
typedef struct {
	uint32_t phys_addr;
	uint16_t byte_count;
	uint16_t flags;
} __attribute__((packed)) PRD;

/* IDE 设备信息结构 */
typedef struct {
	char model[41];
	char serial[21];
	char firmware[9];
	uint64_t sectors;
	uint32_t capabilities;
	bool lba48_supported;
} IDE_DEVICE_INFO;

int ide_identify(bool primary, bool master, IDE_DEVICE_INFO *info);

#ifdef __cplusplus
	}
#endif

#endif