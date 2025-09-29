/*
	devices/blkdev/hd.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/typedef.h>

/* IDE 端口 */
#define IDE_PRIMARY_BASE					0x1F0		/* 主 IDE 控制器基地址 */
#define IDE_SECONDARY_BASE					0x170		/* 从 IDE 控制器基地址 */

/* IDE 驱动器选择 */
#define IDE_DRIVE_MASTER					0xA0		/* 主盘 */
#define IDE_DRIVE_SLAVE						0xB0		/* 从盘 */
#define IDE_DRIVE_LBA_MODE					0x40		/* LBA 模式 */

/* IDE 命令 */
#define IDE_CMD_READ_PIO					0x20		/* PIO 模式读扇区 */
#define IDE_CMD_WRITE_PIO					0x30		/* PIO 模式写扇区 */
#define IDE_CMD_FLUSH_CACHE					0xE7		/* 刷新缓存 */
#define IDE_CMD_IDENTIFY					0xEC		/* 识别设备 */

/* IDE 寄存器偏移 */
#define IDE_REG_DATA						0x0			/* 16 位数据端口 */
#define IDE_REG_ERROR						0x1			/* 错误寄存器（读） */
#define IDE_REG_FEATURES					0x1			/* 功能寄存器 */
#define IDE_REG_SECTOR_COUNT				0x2			/* 扇区计数寄存器 */
#define IDE_REG_LBA_LOW						0x3			/* LBA 0-7 */
#define IDE_REG_LBA_MID						0x4			/* LBA 8-15 */
#define IDE_REG_LBA_HIGH					0x5			/* LBA 16-23 */
#define IDE_REG_DRIVE_SELECT				0x6			/* 设备选择寄存器（LBA 24-27 + 主从选择） */
#define IDE_REG_STATUS						0x7			/* 状态寄存器（读） */
#define IDE_REG_COMMAND						0x7			/* 命令寄存器（写） */

/* IDE 状态寄存器位 */
#define IDE_STATUS_ERR						(1 << 0)	/* 错误 */
#define IDE_STATUS_DRQ						(1 << 3)	/* 数据请求就绪 */
#define IDE_STATUS_SRV						(1 << 4)	/* 服务请求 */
#define IDE_STATUS_DF						(1 << 5)	/* 设备故障 */
#define IDE_STATUS_RDY						(1 << 6)	/* 设备就绪 */
#define IDE_STATUS_BSY						(1 << 7)	/* 设备忙 */

/* 超时时间 */
#define IDE_WAIT_TIMEOUT					1000000

/* IDE 设备全局实例 */
IDE_DEVICE ide_devices[IDE_DEVICE_COUNT] = {
	{ .primary = true, .master = true, .type = IDE_DEVICE_NONE },	/* 主通道主盘 */
	{ .primary = true, .master = false, .type = IDE_DEVICE_NONE },	/* 主通道从盘 */
	{ .primary = false, .master = true, .type = IDE_DEVICE_NONE },	/* 从通道主盘 */
	{ .primary = false, .master = false, .type = IDE_DEVICE_NONE }	/* 从通道从盘 */
};

/* 等待 IDE 状态 */
static int ide_wait_status(bool primary, uint8_t mask, uint8_t value)
{
	uint32_t timeout = IDE_WAIT_TIMEOUT;
	uint16_t base = primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;
	uint8_t status;

	while (timeout--) {
		status = in8(base + IDE_REG_STATUS);
		
		/* 检查错误状态 */
		if (status & (IDE_STATUS_DF | IDE_STATUS_ERR)) {
			uint8_t error = in8(base + IDE_REG_ERROR);
			debug("IDE status error: status=0x%02X, error=0x%02X.\n", status, error);
			return BD_NOT_READY;
		}
		
		if ((status & mask) == value)
			return BD_SUCCESS;
	}
	
	return BD_TIMEOUT;
}

/* 等待 IDE 就绪 */
static int ide_wait_ready(bool primary)
{
	return ide_wait_status(primary, IDE_STATUS_BSY, 0);
}

/* 等待数据请求 */
static int ide_wait_drq(bool primary)
{
	return ide_wait_status(primary, IDE_STATUS_BSY | IDE_STATUS_DRQ, IDE_STATUS_DRQ);
}

/* 识别 IDE 设备 */
static int ide_identify(IDE_DEVICE *dev)
{
	uint16_t base = dev->primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;
	uint16_t buffer[256];

	/* 选择驱动器 */
	out8(base + IDE_REG_DRIVE_SELECT, 
		(dev->master ? IDE_DRIVE_MASTER : IDE_DRIVE_SLAVE) | IDE_DRIVE_LBA_MODE);

	int err = ide_wait_ready(dev->primary);
	if (err != BD_SUCCESS) {
		dev->type = IDE_DEVICE_NONE;
		return err;
	}

	/* 发送 IDENTIFY 命令 */
	out8(base + IDE_REG_COMMAND, IDE_CMD_IDENTIFY);

	/* 等待设备响应 */
	err = ide_wait_drq(dev->primary);
	if (err != BD_SUCCESS) {
		dev->type = IDE_DEVICE_NONE;
		return err;
	}

	/* 读取数据 */
	for (int i = 0; i < 256; i++)
		buffer[i] = in16(base + IDE_REG_DATA);

	/* 解析设备类型 */
	dev->type = (buffer[0] & 0x8000) ? IDE_DEVICE_CDROM : IDE_DEVICE_HD;

	/* 解析型号 */
	for (int i = 0; i < 20; i++) {
		dev->model[i * 2] = (buffer[27 + i] >> 8) & 0xFF;
		dev->model[i * 2 + 1] = buffer[27 + i] & 0xFF;
	}
	dev->model[40] = '\0';

	/* 解析序列号 */
	for (int i = 0; i < 10; i++) {
		dev->serial[i * 2] = (buffer[10 + i] >> 8) & 0xFF;
		dev->serial[i * 2 + 1] = buffer[10 + i] & 0xFF;
	}
	dev->serial[20] = '\0';

	/* 解析固件版本 */
	for (int i = 0; i < 4; i++) {
		dev->firmware[i * 2] = (buffer[23 + i] >> 8) & 0xFF;
		dev->firmware[i * 2 + 1] = buffer[23 + i] & 0xFF;
	}
	dev->firmware[8] = '\0';

	/* 检查 LBA48 支持 */
	dev->lba48_supported = (buffer[83] & 0x400) != 0;

	/* 获取容量 */
	if (dev->lba48_supported)
		dev->sectors = ((uint64_t)buffer[103] << 48) | ((uint64_t)buffer[102] << 32) |
						((uint64_t)buffer[101] << 16) | buffer[100];
	else
		dev->sectors = ((uint32_t)buffer[61] << 16) | buffer[60];

	/* 获取能力信息 */
	dev->capabilities = (buffer[49] << 16) | buffer[0];

	debug("IDE device detected: %s %s, sectors: %llu\n", 
		  dev->model, dev->serial, dev->sectors);
	
	return BD_SUCCESS;
}

/*
	@brief 初始化 IDE 设备。
	@return 成功初始化的设备数量
*/
uint32_t ide_init(void)
{
	int count = 0;
	
	for (int i = 0; i < IDE_DEVICE_COUNT; i++) {
		if (ide_identify(&ide_devices[i]) == BD_SUCCESS) {
			count++;
			debug("IDE device %d initialized: %s\n", i, ide_devices[i].model);
		} else {
			debug("No IDE device at %s %s.\n", 
				  ide_devices[i].primary ? "primary" : "secondary",
				  ide_devices[i].master ? "master" : "slave");
		}
	}
	
	return count;
}

/* 选择 IDE 驱动器 */
static int ide_select_device(IDE_DEVICE *dev, uint32_t lba)
{
	uint16_t base = dev->primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;

	/* 等待设备就绪 */
	int err = ide_wait_ready(dev->primary);
	if (err != BD_SUCCESS)
		return err;

	/* 设置设备选择寄存器（LBA28 模式 + 主从选择 + LBA 24-27） */
	uint8_t dev_sel = 0xE0;			/* BIT6=1（LBA 模式），BIT7=1（强制选择） */
	dev_sel |= (dev->master ? 0 : 1) << 4;	/* BIT4：0=主盘，1=从盘 */
	dev_sel |= (lba >> 24) & 0x0F;	/* BIT0-3：LBA24-LBA27（28 位 LBA 的高 4 位） */
	out8(base + IDE_REG_DRIVE_SELECT, dev_sel);

	/* 等待设备确认选择 */
	in8(base + IDE_REG_STATUS); /* 空读刷新状态 */

	/* 设置 LBA 低 24 位地址和扇区计数 */
	out8(base + IDE_REG_LBA_LOW, lba);			/* LBA 0-7 */
	out8(base + IDE_REG_LBA_MID, lba >> 8);		/* LBA 8-15 */
	out8(base + IDE_REG_LBA_HIGH, lba >> 16);	/* LBA 16-23 */
	out8(base + IDE_REG_SECTOR_COUNT, 1);		/* 默认 1 个扇区*/

	/* 再次等待设备就绪 */
	return ide_wait_ready(dev->primary);
}

/*
	@brief IDE PIO 模式读取扇区。
	@param dev 目标 IDE 设备
	@param lba 起始扇区号
	@param sec_count 读取扇区数量
	@param buf 数据缓冲区
	@return 错误码
*/
int32_t ide_read_sectors(IDE_DEVICE *dev, uint32_t lba, uint32_t sec_count, uint16_t *buf)
{
	/* 检查参数合法性 */
	if (dev == NULL || dev->type == IDE_DEVICE_NONE)
		return BD_NOT_EXIST;
	if (sec_count == 0 || buf == NULL)
		return BD_INVALID_PARAM;
	if (lba + sec_count > dev->sectors)
		return BD_INVALID_PARAM;

	uint16_t base = dev->primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;

	/* 选择设备并设置 LBA 地址 */
	int err = ide_select_device(dev, lba);
	if (err != BD_SUCCESS) return err;

	/* 设置扇区计数 */
	out8(base + IDE_REG_SECTOR_COUNT, sec_count);

	/* 发送读扇区命令（PIO 模式） */
	out8(base + IDE_REG_COMMAND, IDE_CMD_READ_PIO);

	/* 读取每个扇区的数据 */
	for (uint32_t i = 0; i < sec_count; i++) {
		/* 等待当前扇区就绪 */
		err = ide_wait_drq(dev->primary);
		if (err != BD_SUCCESS) return BD_IO_ERROR;

		/* 读取 512 字节 */
		for (int j = 0; j < HD_SECTOR_SIZE / 2; j++)
			*buf++ = in16(base + IDE_REG_DATA);

		/* 空读状态寄存器 */
		in8(base + IDE_REG_STATUS);
	}

	return BD_SUCCESS;
}

/*
	@brief IDE PIO 模式写入扇区。
	@param dev 目标 IDE 设备
	@param lba 起始扇区号
	@param sec_count 写入扇区数量
	@param buf 数据缓冲区
	@return 错误码
*/
int32_t ide_write_sectors(IDE_DEVICE *dev, uint32_t lba, uint32_t sec_count, const uint16_t *buf)
{
	/* 检查参数合法性 */
	if (dev == NULL || dev->type == IDE_DEVICE_NONE)
		return BD_NOT_EXIST;
	if (sec_count == 0 || buf == NULL)
		return BD_INVALID_PARAM;
	if (lba + sec_count > dev->sectors)
		return BD_INVALID_PARAM;

	uint16_t base = dev->primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;

	/* 选择设备并设置 LBA 地址 */
	int err = ide_select_device(dev, lba);
	if (err != BD_SUCCESS) return err;

	/* 设置扇区计数 */
	out8(base + IDE_REG_SECTOR_COUNT, sec_count);

	/* 发送写扇区命令（PIO 模式） */
	out8(base + IDE_REG_COMMAND, IDE_CMD_WRITE_PIO);

	/* 写入每个扇区的数据 */
	for (uint32_t i = 0; i < sec_count; i++) {
		/* 等待当前扇区就绪 */
		err = ide_wait_drq(dev->primary);
		if (err != BD_SUCCESS) return BD_IO_ERROR;

		/* 写入 512 字节 */
		for (int j = 0; j < HD_SECTOR_SIZE / 2; j++)
			out16(base + IDE_REG_DATA, *buf++);

		/* 空读状态寄存器 */
		in8(base + IDE_REG_STATUS);
	}

	return BD_SUCCESS;
}

/*
	@brief 刷新 IDE 设备缓存。
	@param dev 目标 IDE 设备
	@return 错误码
*/
int32_t ide_flush_cache(IDE_DEVICE *dev)
{
	/* 检查参数合法性 */
	if (dev == NULL || dev->type == IDE_DEVICE_NONE)
		return BD_NOT_EXIST;

	uint16_t base = dev->primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;

	/* 等待设备就绪 */
	int err = ide_wait_ready(dev->primary);
	if (err != BD_SUCCESS) return err;

	/* 发送刷新缓存命令 */
	out8(base + IDE_REG_COMMAND, IDE_CMD_FLUSH_CACHE);

	/* 等待命令完成 */
	return ide_wait_ready(dev->primary);
}