/*
	devices/blkdev/fd.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/io.h>
#include <ClassiX/pit.h>
#include <ClassiX/typedef.h>

#define FDC_DOR								0x3f2		/* -W 数字输出寄存器 */
#define FDC_STATUS							0x3f4		/* R- 软盘状态寄存器 */
#define FDC_DATA							0x3f5		/* RW 软盘数据寄存器 */
#define FDC_DIR								0x3f7		/* R- 数字输入寄存器 */
#define FDC_DCR								0x3f7		/* -W 磁盘控制寄存器 */

#define FDC_DOR_DRIVE_A						0b00000000	/* 选择驱动器 A */
#define FDC_DOR_DRIVE_B						0b00000001	/* 选择驱动器 B */
#define FDC_DOR_DRIVE_C						0b00000010	/* 选择驱动器 C */
#define FDC_DOR_DRIVE_D						0b00000011	/* 选择驱动器 D */
#define FDC_DOR_RESET						0b00000100	/* 重置 FDC */
#define FDC_DOR_DMAINT						0b00001000	/* DMA 中断 */
#define FDC_DOR_MOTOR_0						0b00010000	/* 驱动器 A 电机 */
#define FDC_DOR_MOTOR_1						0b00100000	/* 驱动器 B 电机 */
#define FDC_DOR_MOTOR_2						0b01000000	/* 驱动器 C 电机 */
#define FDC_DOR_MOTOR_3						0b10000000	/* 驱动器 D 电机 */

#define FDC_STATUS_DAB						0b00000001	/* 驱动器 A 忙 */
#define FDC_STATUS_DBB						0b00000010	/* 驱动器 B 忙 */
#define FDC_STATUS_DCB						0b00000100	/* 驱动器 C 忙 */
#define FDC_STATUS_DDB						0b00001000	/* 驱动器 D 忙 */
#define FDC_STATUS_CB						0b00010000	/* 控制器忙 */
#define FDC_STATUS_NDM						0b00100000	/* DMA 设置 0 - 启用 */
#define FDC_STATUS_DIO						0b01000000	/* 传输方向 0 - CPU->FDD */
#define FDC_STATUS_RQM						0b10000000	/* 数据就绪 */

#define FD_SPECIFY							0b00000011	/* 设定驱动器参数 */
#define FD_RECALIBRATE						0b00000111	/* 重新校正 */
#define FD_SEEK								0b00001111	/* 寻道 */
#define FD_READ								0b11100110	/* 读扇区 */
#define FD_WRITE							0b11000101	/* 写扇区 */
#define FD_SENSEI							0b00001000	/* 检测中断状态 */

#define FD_TIMEOUT							1000000		/* 超时 */

/* 状态寄存器位定义 */
typedef struct __attribute__((packed)) {
	uint8_t dab:1;	/* 驱动器 A 忙 */
	uint8_t dbb:1;	/* 驱动器 B 忙 */
	uint8_t dcb:1;	/* 驱动器 C 忙 */
	uint8_t ddb:1;	/* 驱动器 D 忙 */
	uint8_t cb:1;	/* 控制器忙 */
	uint8_t ndm:1;	/* DMA 设置 0-DMA */
	uint8_t dio:1;	/* 传输方向 0-CPU->FDD; 1-FDD->CPU */
	uint8_t rqm:1;	/* 数据就绪 1-ready */
} STATUS;

/* ST0 寄存器位定义 */
typedef struct __attribute__((packed)) {
	uint8_t ds:2;		/* 驱动器号 */
	uint8_t ha:1;		/* 磁头地址 */
	uint8_t nr:1;		/* 磁盘未就绪 */
	uint8_t ece:1;		/* 设备检查错误 */
	uint8_t se:1;		/* 寻道 / 重新校正操作结束 */
	uint8_t intr:2;		/* 中断原因 */
} ST0;

/* ST1 寄存器位定义 */
typedef struct __attribute__((packed)) {
	uint8_t mam:1;		/* 缺少地址标记 */
	uint8_t wp:1;		/* 写保护 */
	uint8_t nd:1;		/* 无数据 (扇区未找到) */
	uint8_t resv1:1;	/* 保留 */
	uint8_t or:1;		/* 超限/欠载 */
	uint8_t crc:1;		/* CRC 错误 */
	uint8_t resv2:2;	/* 保留 */
	uint8_t eoc:1;		/* 柱面结束 */
} ST1;

/* ST2 寄存器位定义 */
typedef struct __attribute__((packed)) {
	uint8_t mam:1;		/* 缺少地址标记 */
	uint8_t bc:1;		/* 坏柱面 */
	uint8_t sns:1;		/* 扫描不满足 */
	uint8_t seh:1;		/* 扫描相等命中 */
	uint8_t wc:1;		/* 错误柱面 (ID 字段 CRC 错误) */
	uint8_t crc:1;		/* 数据字段 CRC 错误 */
	uint8_t cm:1;		/* 控制标记 (删除数据标记) */
	uint8_t resv:1;	/* 保留 */
} ST2;

#define FD_SPECIFY_LOWER(arg)				(*((uint8_t*) &(arg)))			/* 获取低字节 */
#define FD_SPECIFY_UPPER(arg)				(*((uint8_t*) (&(arg) + 1)))	/* 获取高字节 */

static void fdc_dma_setup(uint8_t *buf, uint32_t count, uint8_t cmd);

/* 重置 FDC */
static void fdc_reset(uint8_t drive)
{
	const uint8_t drive_sel[] = {
		FDC_DOR_DRIVE_A,
		FDC_DOR_DRIVE_B,
		FDC_DOR_DRIVE_C,
		FDC_DOR_DRIVE_D
	};

	uint8_t dor = 0b00001000; /* 启用 DMA 中断，重置 FDC */
	dor |= drive_sel[drive & 0x03]; /* 选择驱动器 */
	out8(FDC_DOR, dor);
	delay(10); /* 等待 10ms */

	dor &= ~0b00000100; /* 取消重置 FDC */
	out8(FDC_DOR, dor);
}

/* 向 FDC 写入数据 */
static int32_t fdc_write_byte(uint8_t data)
{
	for (uint32_t i = 0; i < FD_TIMEOUT; i++) {
		uint8_t _status = in8(FDC_STATUS);
		STATUS status = *((STATUS*) &_status);
		if (status.rqm && !status.dio) {
			out8(FDC_DATA, data);
			return BD_SUCCESS;
		}
	}

	return BD_TIMEOUT; /* 超时 */
}

/* 从 FDC 读取数据 */
static int32_t fdc_read_byte(uint8_t* data)
{
	for (uint32_t i = 0; i < FD_TIMEOUT; i++) {
		uint8_t _status = in8(FDC_STATUS);
		STATUS status = *((STATUS*) &_status);
		if (status.rqm && status.dio) {
			*data = in8(FDC_DATA);
			return BD_SUCCESS;
		}
	}

	return BD_TIMEOUT; /* 超时 */
}

/* 设定 FDC 参数 */
static void fdc_specify(void)
{
	fdc_write_byte(FD_SPECIFY);
	fdc_write_byte(0xCF);		/* SRT=8ms, HUT=512ms */
	fdc_write_byte(0x03 << 1);	/* HLT=16ms, 启用 DMA */
}

/* 重新校正 */
static void fdc_recalibrate(uint8_t drive)
{
	fdc_write_byte(FD_RECALIBRATE);
	fdc_write_byte(drive & 0x03);
}

/* 寻道 */
static void fdc_seek(uint8_t drive, uint8_t head, uint8_t cylinder)
{
	fdc_write_byte(FD_SEEK);
	fdc_write_byte(((head & 0x01) << 2) | (drive & 0x03));
	fdc_write_byte(cylinder);
}

/* 检测中断状态 */
static int32_t fdc_sense_interrupt(ST0 *st0, uint8_t *cyl)
{
	fdc_write_byte(FD_SENSEI);

	uint8_t status_byte;
	int32_t ret = fdc_read_byte(&status_byte);
	if (ret != BD_SUCCESS)
		return ret;

	*st0 = *((ST0*) &status_byte);
	ret = fdc_read_byte(cyl);
	return ret;
}

/* LBA 转 CHS */
static inline void lba_to_chs(uint32_t lba, uint8_t *cylinder, uint8_t *head, uint8_t *sector)
{
	*cylinder = (lba / (2 * 18));
	*head     = (lba / 18) % 2;
	*sector   = (lba % 18) + 1;
}

/* 读扇区 */
static int32_t fdc_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer)
{
	uint8_t cylinder, head, sector;
	lba_to_chs(lba, &cylinder, &head, &sector);

	if (drive > 3 || cylinder > 79 || head > 1 || sector > 18)
		return BD_INVALID_PARAM; /* 参数无效 */
	
	/* 重新校正 */
	fdc_recalibrate(drive);

	/* 寻道 */
	fdc_seek(drive, head, cylinder);

	/* 设置 DMA */
	fdc_dma_setup(buffer, 512, FD_READ);

	/* 发送读扇区命令 */
	fdc_write_byte(FD_READ);		/* 读扇区命令 */
	fdc_write_byte(drive & 0x03);	/* 驱动器号 */
	fdc_write_byte(cylinder);		/* 柱面号 */
	fdc_write_byte(head);			/* 磁头号 */
	fdc_write_byte(sector);			/* 开始扇区号 */
	fdc_write_byte(2);				/* 每扇区字节数 512 */
	fdc_write_byte(18);				/* 每轨扇区数 18 */
	fdc_write_byte(0x1B);			/* GAP3 长度 */
	fdc_write_byte(0xFF);			/* 传输长度 */

	return BD_SUCCESS;
}

/* 写扇区 */
static int32_t fdc_write_sector(uint8_t drive, uint32_t lba, uint8_t *buffer)
{
	uint8_t cylinder, head, sector;
	lba_to_chs(lba, &cylinder, &head, &sector);

	if (drive > 3 || cylinder > 79 || head > 1 || sector > 18)
		return BD_INVALID_PARAM; /* 参数无效 */
	
	/* 重新校正 */
	fdc_recalibrate(drive);

	/* 寻道 */
	fdc_seek(drive, head, cylinder);

	/* 设置 DMA */
	fdc_dma_setup(buffer, 512, FD_WRITE);

	/* 发送写扇区命令 */
	fdc_write_byte(FD_WRITE);		/* 写扇区命令 */
	fdc_write_byte(drive & 0x03);	/* 驱动器号 */
	fdc_write_byte(cylinder);		/* 柱面号 */
	fdc_write_byte(head);			/* 磁头号 */
	fdc_write_byte(sector);			/* 开始扇区号 */
	fdc_write_byte(2);				/* 每扇区字节数 512 */
	fdc_write_byte(18);				/* 每轨扇区数 18 */
	fdc_write_byte(0x1B);			/* GAP3 长度 */
	fdc_write_byte(0xFF);			/* 传输长度 */

	return BD_SUCCESS;
}

/* 8 位带延迟输出 */
#define immout8_p(val, port) \
	asm volatile ("outb %0, %1\njmp 1f\n1:jmp 1f\n1:"::"a"((uint8_t) (val)), "Nd"((uint16_t) (port)))

/* 16 位带延迟输出 */  
#define immout16_p(val, port) \
	asm volatile ("outw %0, %1\njmp 1f\n1:jmp 1f\n1:"::"a"((uint16_t) (val)), "Nd"((uint16_t) (port)))

/* 32 位带延迟输出 */
#define immout32_p(val, port) \
	asm volatile ("outl %0, %1\njmp 1f\n1:jmp 1f\n1:"::"a"((uint32_t) (val)), "Nd"((uint16_t) (port)))

#define DMA_READ							0x46 	/* DMA 读命令 */
#define DMA_WRITE							0x4A	/* DMA 写命令 */

/* 设置 DMA 传输 */
static void fdc_dma_setup(uint8_t *buf, uint32_t count, uint8_t cmd)
{
	uint32_t addr = (uint32_t) buf;

	/* 初始化 */
	cli();
	immout8_p(4 | 2, 0x0A);						/* 禁用通道 2 */
	immout8_p(0xFF, 0x0C);						/* 清除翻转寄存器 */

	/* 设置 DMA 模式 */
	uint8_t outcmd = cmd == FD_READ ? DMA_READ : DMA_WRITE;
	immout8_p(outcmd, 0x0C);					/* 设置 DMA 命令寄存器 */
	immout8_p(outcmd, 0x0B);

	/* 设置内存地址 */
	immout8_p(addr & 0xFF, 0x04);				/* 地址低字节 */
	immout8_p((addr >> 8) & 0xFF, 0x04);		/* 地址中字节 */
	immout8_p((addr >> 16) & 0xFF, 0x81);		/* 地址高字节 */

	/* 设置传输长度 */
	immout8_p((count - 1) & 0xFF, 0x05);		/* 长度低字节 */
	immout8_p(((count - 1) >> 8) & 0xFF, 0x05);	/* 长度高字节 */

	/* 启动 DMA */
	immout8_p(0 | 2, 0x0A);						/* 启用通道 2 */
	sti();
}
