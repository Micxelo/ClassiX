/*
	devices/blkdev/fd.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/pit.h>
#include <ClassiX/typedef.h>

/* FDC 端口 */
#define FDC_BASE							0x3F0	/* 软驱控制器基地址 */
#define FDC_DOR								0x3F2	/* 数字输出寄存器 */
#define FDC_MSR								0x3F4	/* 主状态寄存器 */
#define FDC_FIFO							0x3F5	/* 数据 FIFO 端口 */
#define FDC_CCR								0x3F7	/* 数字输入寄存器/配置控制寄存器 */

/* DOR 位 */
#define DOR_DRIVE0							0x00	/* 选择驱动器 0 */
#define DOR_DRIVE1							0x01	/* 选择驱动器 1 */
#define DOR_DRIVE2							0x02	/* 选择驱动器 2 */
#define DOR_DRIVE3							0x03	/* 选择驱动器 3 */
#define DOR_RESET							0x04	/* 复位控制器 */
#define DOR_DMA_ENABLE						0x08	/* DMA、中断使能 */
#define DOR_MOTOR_DRIVE0					0x10	/* 驱动器 0 电机使能 */
#define DOR_MOTOR_DRIVE1					0x20	/* 驱动器 1 电机使能 */
#define DOR_MOTOR_DRIVE2					0x40	/* 驱动器 2 电机使能 */
#define DOR_MOTOR_DRIVE3					0x80	/* 驱动器 3 电机使能 */

/* MSR 位 */
#define MSR_DRIVE0_BUSY						0x01	/* 驱动器 0 忙 */
#define MSR_DRIVE1_BUSY						0x02	/* 驱动器 1 忙 */
#define MSR_DRIVE2_BUSY						0x04	/* 驱动器 2 忙 */
#define MSR_DRIVE3_BUSY						0x08	/* 驱动器 3 忙 */
#define MSR_FDC_BUSY						0x10	/* 控制器忙 */
#define MSR_NON_DMA							0x20	/* 非 DMA 模式 */
#define MSR_DATA_IO							0x40	/* 数据传输方向：1=CPU->FDC，0=FDC->CPU */
#define MSR_DATA_READY						0x80	/* 数据寄存器就绪 */

/* FDC 命令 */
#define FDC_CMD_READ						0x06	/* 读扇区 */
#define FDC_CMD_WRITE						0x05	/* 写扇区 */
#define FDC_CMD_SEEK						0x0F	/* 寻道 */
#define FDC_CMD_RECALIBRATE					0x07	/* 磁头归位 */
#define FDC_CMD_SENSE_INTERRUPT				0x08	/* 中断状态感知 */
#define FDC_CMD_SPECIFY						0x03	/* 参数设定 */
#define FDC_CMD_CONFIGURE					0x13	/* 控制器配置 */

/* FDC 状态寄存器 0 位定义 */
#define ST0_INTERRUPT_CODE_MASK				0xC0	/* 中断代码掩码 */
#define ST0_SEEK_END						0x20	/* 寻道结束 */
#define ST0_EQUIPMENT_CHECK					0x10	/* 设备检查错误 */
#define ST0_NOT_READY						0x08	/* 设备未就绪 */
#define ST0_HEAD_ACTIVE						0x04	/* 磁头地址 */
#define ST0_UNIT_SELECT_MASK				0x03	/* 驱动器选择掩码 */

/* 驱动器类型 */
#define DRIVE_TYPE_NONE						0		/* 无驱动器 */
#define DRIVE_TYPE_360KB					1		/* 360KB 软盘 */
#define DRIVE_TYPE_1_2MB					2		/* 1.2MB 软盘 */
#define DRIVE_TYPE_720KB					3		/* 720KB 软盘 */
#define DRIVE_TYPE_1_44MB					4		/* 1.44MB 软盘 */
#define DRIVE_TYPE_2_88MB					5		/* 2.88MB 软盘 */

FD_DEVICE fd_devices[FD_DEVICE_COUNT] = {
	{ .id = 0, .type = DRIVE_TYPE_NONE, .present = 0, .motor = 0 }, /* 驱动器 0 */
	{ .id = 1, .type = DRIVE_TYPE_NONE, .present = 0, .motor = 0 }, /* 驱动器 1 */
	{ .id = 2, .type = DRIVE_TYPE_NONE, .present = 0, .motor = 0 }, /* 驱动器 2 */
	{ .id = 3, .type = DRIVE_TYPE_NONE, .present = 0, .motor = 0 }  /* 驱动器 3 */
};

static bool fdc_detect(void);
static int32_t fdc_detect_disk(uint32_t drive);
static int32_t fdc_wait_ready(void);
static int32_t fdc_send_command(uint8_t cmd);
static int32_t fdc_read_data(uint8_t *data);
static int32_t fdc_write_data(uint8_t data);
static void fdc_motor_on(uint32_t drive);
static void fdc_motor_off(uint32_t drive);
static int32_t fdc_reset(void);
static int32_t fd_seek(uint32_t drive, uint8_t cylinder, uint8_t head);
static int32_t lba_to_chs(uint32_t lba, uint32_t *cylinder, uint32_t *head, uint32_t *sector);

/* 检测 FDC 是否存在 */
static bool fdc_detect(void)
{
	uint8_t dor0;
	const uint8_t dor = DOR_DMA_ENABLE | DOR_RESET | DOR_MOTOR_DRIVE0;
	bool res = false;
	
	dor0 = in8(FDC_DOR); /* 保存 DOR 原始值 */
	out8(FDC_DOR, dor); /* 尝试写入测试值 */

	delay(10); /* 等待 10ms */

	/* 回读 DOR */
	if (in8(FDC_DOR) == dor) /* 写入成功 */
		res = true;
	out8(FDC_DOR, dor0); /* 恢复原值 */

	return res;
}

static int32_t fdc_detect_disk(uint32_t drive)
{
	if (drive > 3) return BD_INVALID_PARAM; /* 仅支持 4 个驱动器 */

	/* 启动电机 */
	fdc_motor_on(drive);
	delay(500); /* 等待电机启动 */

	/* 发送 SENSE INTERRUPT 命令 */
	if (fdc_send_command(FDC_CMD_SENSE_INTERRUPT) != BD_SUCCESS) {
		fdc_motor_off(drive);
		return BD_NOT_READY;
	}

	/* 读取状态寄存器 0 和磁头位置 */
	uint8_t st0, cyl;
	if (fdc_read_data(&st0) != BD_SUCCESS || fdc_read_data(&cyl) != BD_SUCCESS) {
		fdc_motor_off(drive);
		return BD_IO_ERROR;
	}

	/* 检查驱动器是否就绪 */
	if (st0 & ST0_NOT_READY) {
		fdc_motor_off(drive);
		fd_devices[drive].present = 0;
		return BD_NOT_READY; /* 驱动器未就绪 */
	}

	fd_devices[drive].present = 1; /* 驱动器存在 */
	fdc_motor_off(drive); /* 关闭电机 */
	return BD_SUCCESS;
}

/* 等待 FDC 就绪 */
static int32_t fdc_wait_ready(void)
{
	int timeout = 1000000;

	/* 等待 MSR 数据寄存器就绪 */
	while (timeout--) 
		if (in8(FDC_BASE + FDC_MSR) & MSR_DATA_READY)
			return BD_SUCCESS;

	return BD_TIMEOUT; /* 超时 */
}

/* 向 FDC 发送命令 */
static int32_t fdc_send_command(uint8_t cmd)
{
	int err = fdc_wait_ready();
	if (err != BD_SUCCESS) return err;

	/* 检查是否可以发送数据 */
	uint8_t msr = in8(FDC_MSR);
	if ((msr & (MSR_DATA_IO | MSR_DATA_READY)) != MSR_DATA_READY)
		return BD_NOT_READY;

	out8(FDC_BASE + FDC_FIFO, cmd);
	return BD_SUCCESS;
}

/* 从 FDC 读取数据 */
static int32_t fdc_read_data(uint8_t *data)
{
	int err = fdc_wait_ready();
	if (err != BD_SUCCESS) return err;

	/* 检查是否可以读取数据 */
	uint8_t msr = in8(FDC_MSR);
	if ((msr & (MSR_DATA_IO | MSR_DATA_READY)) != (MSR_DATA_IO | MSR_DATA_READY))
		return BD_NOT_READY;

	*data = in8(FDC_BASE + FDC_FIFO);
	return BD_SUCCESS;
}

/* 向 FDC 写入数据 */
static int32_t fdc_write_data(uint8_t data)
{
	int err = fdc_wait_ready();
	if (err != BD_SUCCESS) return err;

	/* 确保 FDC 准备好接收数据 */
	uint8_t msr = in8(FDC_MSR);
	if ((msr & (MSR_DATA_IO | MSR_DATA_READY)) != MSR_DATA_READY)
		return BD_NOT_READY;

	out8(FDC_BASE + FDC_FIFO, data);
	return BD_SUCCESS;
}

/* 启动指定驱动器的电机 */
static void fdc_motor_on(uint32_t drive)
{
	if (drive > 3) return; /* 仅支持 4 个驱动器 */
	if (fd_devices[drive].motor) return; /* 电机已启动 */

	uint8_t dor = in8(FDC_DOR);
	dor |= DOR_DMA_ENABLE | DOR_RESET; /* 复位控制器，启用 DMA 和中断 */

	/* 设置指定驱动器的电机位 */
	const uint8_t motor_bits[] = { DOR_MOTOR_DRIVE0, DOR_MOTOR_DRIVE1, DOR_MOTOR_DRIVE2, DOR_MOTOR_DRIVE3 };
	dor |= motor_bits[drive];

	out8(FDC_DOR, dor);

	fd_devices[drive].motor = true; /* 标记电机已启动 */
}

/* 关闭指定驱动器的电机 */
static void fdc_motor_off(uint32_t drive)
{
	if (drive > 3) return; /* 仅支持 4 个驱动器 */
	if (!fd_devices[drive].motor) return; /* 电机已关闭 */

	uint8_t dor = in8(FDC_DOR);

	/* 清除指定驱动器的电机位 */
	const uint8_t motor_bits[] = { DOR_MOTOR_DRIVE0, DOR_MOTOR_DRIVE1, DOR_MOTOR_DRIVE2, DOR_MOTOR_DRIVE3 };
	dor &= ~motor_bits[drive];

	out8(FDC_DOR, dor);

	fd_devices[drive].motor = false; /* 标记电机已关闭 */
}

/* 复位 FDC 控制器 */
static int32_t fdc_reset(void)
{
	/* 复位 FDC */
	uint8_t dor = in8(FDC_DOR);
	out8(FDC_DOR, dor & ~DOR_RESET); /* 复位 */
	delay(10); /* 等待 10ms */
	out8(FDC_DOR, dor | DOR_RESET); /* 取消复位 */

	/* 等待控制器就绪 */
	if (fdc_wait_ready() != BD_SUCCESS)
		return BD_NOT_READY;

	/* 清除中断状态 */
	for (int i = 0; i < 4; i++) {
		fdc_send_command(FDC_CMD_SENSE_INTERRUPT);
		uint8_t st0, cyl;
		fdc_read_data(&st0);
		fdc_read_data(&cyl);
	}

	/* 配置 FDC */
	uint8_t specify_cmd[] = { FDC_CMD_SPECIFY, 0xdf, 0x02 }; /* 步进速率=3ms，磁头卸载时间=240ms */
	for (int i = 0; i < (signed) (sizeof(specify_cmd) / sizeof(specify_cmd[0])); i++)
		if (fdc_write_data(specify_cmd[i]) != BD_SUCCESS)
			return BD_IO_ERROR;

	return BD_SUCCESS;
}

/* 寻道到指定柱面和磁头 */
static int32_t fd_seek(uint32_t drive, uint8_t cylinder, uint8_t head)
{
	if (drive > 3 || head >= FD_HEADS || cylinder >= FD_CYLINDERS) 
		return BD_INVALID_PARAM; /* 参数无效 */

	/* 启动电机 */
	fdc_motor_on(drive);
	delay(500); /* 等待电机启动 */

	uint8_t seek_cmd[] = { FDC_CMD_SEEK, (uint8_t) ((drive & 0x03) | (head << 2)), cylinder };
	for (int i = 0; i < (signed) (sizeof(seek_cmd) / sizeof(seek_cmd[0])); i++)
		if (fdc_write_data(seek_cmd[i]) != BD_SUCCESS) {
			fdc_motor_off(drive);
			return BD_IO_ERROR;
		}
	
	/* 等待寻道完成 */
	if (fdc_wait_ready() != BD_SUCCESS) {
		fdc_motor_off(drive);
		return BD_TIMEOUT;
	}

		/* 检查寻道结果 */
	fdc_send_command(FDC_CMD_SENSE_INTERRUPT);
	uint8_t st0 = 0, cyl = 0;
	fdc_read_data(&st0);
	fdc_read_data(&cyl);

	if (st0 & ST0_EQUIPMENT_CHECK || cyl != cylinder) {
		fdc_motor_off(drive);
		return BD_IO_ERROR;
	}

	return BD_SUCCESS;
}

/* LBA 转 CHS */
static inline int32_t lba_to_chs(uint32_t lba, uint32_t *cylinder, uint32_t *head, uint32_t *sector)
{
	if (lba >= FD_TOTAL_SECTORS)
		return BD_INVALID_PARAM; /* LBA 超出范围 */

	*sector = (lba % FD_SECTORS_PER_TRACK) + 1; /* 扇区号从 1 开始 */
	*head = (lba / FD_SECTORS_PER_TRACK	) % FD_HEADS;
	*cylinder = lba / (FD_SECTORS_PER_TRACK * FD_HEADS);

	return BD_SUCCESS;
}

/*
	@brief 初始化软盘设备。
	@return 成功初始化的设备数量
*/
uint32_t fd_init(void)
{
	int count = 0;

	/* 检测 FDC 是否存在 */
	if (!fdc_detect()) {
		debug("FDC not detected.\n");
		return 0; /* FDC 不存在 */
	}

	/* 复位 FDC 控制器 */
	if (fdc_reset() != BD_SUCCESS) {
		debug("FDC reset failed.\n");
		return 0; /* FDC 复位失败 */
	}

	/* 检测所有驱动器 */
	for (int i = 0; i < FD_DEVICE_COUNT; i++) {
		if (fdc_detect_disk(i) == BD_SUCCESS) {
			count++;
			debug("FD Drive %d detected.\n", i);
			fd_devices[i].type = DRIVE_TYPE_1_44MB; /* 假设为 1.44MB 软盘 */
		} else {
			debug("FD Drive %d not present.\n", i);
			fd_devices[i].type = DRIVE_TYPE_NONE; /* 无驱动器 */
		}
	}

	return count;
}

int32_t fd_read_sectors(FD_DEVICE *dev, uint32_t lba, uint32_t count, void *buf)
{
	if (dev == NULL || count == 0 || buf == NULL)
		return BD_INVALID_PARAM; /* 参数无效 */
	if (dev->id >= FD_DEVICE_COUNT)
		return BD_INVALID_PARAM; /* 参数无效 */

	int err;

	uint32_t cylinder, head, sector;
	err = lba_to_chs(lba, &cylinder, &head, &sector);
	if (err != BD_SUCCESS) return err; /* LBA 转换失败 */

	err = fd_seek(dev->id, cylinder, head);
	if (err != BD_SUCCESS) return err; /* 寻道失败 */

	for (int i = 0; i < (signed) count; i++) {
		/* 发送读扇区命令 */
		uint8_t read_cmd[] = {
			FDC_CMD_READ,
			(uint8_t) ((dev->id & 0x03) | (head << 2)), /* 驱动器和磁头 */
			(uint8_t) cylinder,							/* 柱面 */
			(uint8_t) head,								/* 磁头 */
			(uint8_t) sector,							/* 扇区 */
			0x02,										/* 每扇区 512 字节 */
			(uint8_t) FD_SECTORS_PER_TRACK,				/* 每磁道扇区数 */
			0x1B,										/* GAP3 长度 */
			0xFF										/* 数据长度 */
		};
		
		for (int j = 0; j < (signed) (sizeof(read_cmd) / sizeof(read_cmd[0])); j++) {
			if (fdc_write_data(read_cmd[j]) != BD_SUCCESS) {
				fdc_motor_off(dev->id);
				return BD_IO_ERROR; /* 写入命令失败 */
			}
		}

		/* 等待数据就绪 */
		err = fdc_wait_ready();
		if (err != BD_SUCCESS) {
			fdc_motor_off(dev->id);
			return err;
		}

		/* 读取数据 */
		uint16_t *buf16 = (uint16_t *) buf + i * (FD_SECTOR_SIZE / 2);
		for (int j = 0; j < (FD_SECTOR_SIZE / 2); j++) {
			uint8_t low, high;
			if (fdc_read_data(&low) != BD_SUCCESS || fdc_read_data(&high) != BD_SUCCESS) {
				fdc_motor_off(dev->id);
				return BD_IO_ERROR; /* 读取数据失败 */
			}
			buf16[j] = (high << 8) | low;
		}

		/* 读取结果寄存器 */
		uint8_t st0 = 0, st1 = 0, st2 = 0, rcy = 0, rhe = 0, rse = 0;
		fdc_read_data(&st0);
		fdc_read_data(&st1);
		fdc_read_data(&st2);
		fdc_read_data(&rcy);
		fdc_read_data(&rhe);
		fdc_read_data(&rse);
		
		/* 检查错误 */
		if (st0 & 0xC0 || st1 || st2) {
			fdc_motor_off(dev->id);
			return BD_IO_ERROR;
		}

		/* 更新下一个扇区 */
		sector++;
		if (sector > FD_SECTORS_PER_TRACK) {
			sector = 1;
			head++;
			if (head >= FD_HEADS) {
				head = 0;
				cylinder++;
				if (cylinder >= FD_CYLINDERS) {
					/* 超出范围 */
					fdc_motor_off(dev->id);
					return BD_INVALID_PARAM;
				}
				fd_seek(dev->id, cylinder, head);
			}
		}
	}

	fdc_motor_off(dev->id); /* 关闭电机 */
	return BD_SUCCESS;
}
