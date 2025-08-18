/*
	devices/blkdev/harddisk.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/io.h>
#include <ClassiX/memory.h>
#include <ClassiX/pci.h>
#include <ClassiX/pit.h>
#include <ClassiX/typedef.h>

/* ATA 端口定义 */
#define ATA_PRIMARY_BASE					0x1F0		/* 主 ATA 控制器基地址 */
#define ATA_SECONDARY_BASE					0x170		/* 从 ATA 控制器基地址 */

/* ATA 寄存器偏移 */
#define ATA_REG_DATA						0x0			/* 16 位数据端口 */
#define ATA_REG_ERROR						0x1			/* 错误寄存器（读） */
#define ATA_REG_FEATURES					0x1			/* 功能寄存器 */
#define ATA_REG_SECTOR_COUNT				0x2			/* 扇区计数寄存器 */
#define ATA_REG_LBA_LOW						0x3			/* LBA 0-7 */
#define ATA_REG_LBA_MID						0x4			/* LBA 8-15 */
#define ATA_REG_LBA_HIGH					0x5			/* LBA 16-23 */
#define ATA_REG_DRIVE_SELECT				0x6			/* 设备选择寄存器（LBA 24-27 + 主从选择） */
#define ATA_REG_STATUS						0x7			/* 状态寄存器（读） */
#define ATA_REG_COMMAND						0x7			/* 命令寄存器（写） */

/* ATA 状态寄存器位 */
#define ATA_STATUS_ERR						(1 << 0)	/* 错误 */
#define ATA_STATUS_DRQ						(1 << 3)	/* 数据请求就绪 */
#define ATA_STATUS_SRV						(1 << 4)	/* 服务请求 */
#define ATA_STATUS_DF						(1 << 5)	/* 设备故障 */
#define ATA_STATUS_RDY						(1 << 6)	/* 设备就绪 */
#define ATA_STATUS_BSY						(1 << 7)	/* 设备忙 */

/* ATA 命令 */
#define ATA_CMD_IDENTIFY					0xEC		/* 设备信息 */
#define ATA_CMD_READ_SECTOR					0x20		/* PIO 读扇区（LBA28 模式） */
#define ATA_CMD_WRITE_SECTOR				0x30		/* PIO 写扇区（LBA28 模式） */
#define ATA_CMD_FLUSH_CACHE					0xE7		/* 刷新缓存 */

/* ATA 驱动器选择 */
#define ATA_DRIVE_MASTER					0xA0	/* 主盘 */
#define ATA_DRIVE_SLAVE						0xB0	/* 从盘 */
#define ATA_DRIVE_LBA_MODE					0x40	/* LBA 模式 */

#define ATA_WAIT_TIMEOUT					1000000		/* 等待超时阈值 */

/* 等待 DRQ */
static ATA_RESULT ata_wait_drq(bool primary)
{
	uint32_t timeout = ATA_WAIT_TIMEOUT;
	uint16_t base = primary ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
	uint8_t status;
	while (timeout--) {
		status = in8(base + ATA_REG_STATUS);
		if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ))
			return ATA_SUCCESS;
		
		/* 检查错误状态 */
		if (status & (ATA_STATUS_DF | ATA_STATUS_ERR)) {
			uint8_t error = in8(base + ATA_REG_ERROR);
			debug("ATA DRQ error: Status=0x%02X, Error=0x%02X.\n", status, error);
			return ATA_ERR_DEVICE_NOT_READY;
		}
	}
	return ATA_ERR_TIMEOUT;
}

/* 等待 ATA 就绪 */
static ATA_RESULT ata_wait_ready(bool primary)
{
	uint32_t timeout = ATA_WAIT_TIMEOUT;
	uint16_t base = primary ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
	uint8_t status;

	while (timeout) {
		status = in8(base + ATA_REG_STATUS);
		if (!(status & ATA_STATUS_BSY)) break;
		timeout--;
	}
	
	if (timeout == 0) return ATA_ERR_TIMEOUT; /* 等待超时 */

	if (status & (ATA_STATUS_DF | ATA_STATUS_ERR)) {
		/* 错误处理 */
		uint8_t error = in8(base + ATA_REG_ERROR);
		debug("ATA error: Status=0x%02X, Error=0x%02X.\n", status, error);
		return ATA_ERR_DEVICE_NOT_READY;
	}

	return ATA_SUCCESS;
}

/*
	@brief 获取并解析 ATA 设备信息。
	@param primary 是否为主通道
	@param master 是否为主盘
	@param info 指向 ATA_DEVICE_INFO 结构体的指针
	@return ATA_RESULT 错误码
*/
ATA_RESULT ata_identify(bool primary, bool master, ATA_DEVICE_INFO *info)
{
	uint16_t base = primary ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
	uint16_t buffer[256];

	/* 选择驱动器（标准写法：主盘0xA0，从盘0xB0，LBA模式0x40） */
	out8(base + ATA_REG_DRIVE_SELECT, (master ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE) | ATA_DRIVE_LBA_MODE);

	ATA_RESULT err = ata_wait_ready(primary);
	if (err != ATA_SUCCESS) return err;

	/* 发送 IDENTIFY 命令 */
	out8(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

	/* 等待设备响应 */
	uint32_t timeout = ATA_WAIT_TIMEOUT;
	uint8_t status = in8(base + ATA_REG_STATUS);
	do {
		status = in8(base + ATA_REG_STATUS);
		if (status & ATA_STATUS_ERR) {
			/* 处理错误 */
			uint8_t error = in8(base + ATA_REG_ERROR);
			debug("ATA IDENTIFY error: Status=0x%02X, Error=0x%02X.\n", status, error);
		}
	} while (!(status & ATA_STATUS_DRQ) && timeout--);  /* 等待 DRQ 就绪 */
	if (!(status & ATA_STATUS_DRQ)) return ATA_ERR_DEVICE_NOT_READY;

	ata_wait_ready(primary);
	status = in8(base + ATA_REG_STATUS);

	if (status & ATA_STATUS_ERR) {
		/* 错误处理 */
		uint8_t error = in8(base + ATA_REG_ERROR);
		debug("ATA IDENTIFY error: Status=0x%02X, Error=0x%02X.\n", status, error);
		return ATA_ERR_DEVICE_NOT_READY;
	}

	/* 读取数据 */
	for (int i = 0; i < 256; i++)
		buffer[i] = in16(base + ATA_REG_DATA);

	/* 解析型号 */
	for (int i = 0; i < 20; i++) {
		info->model[i * 2] = (buffer[27 + i] >> 8) & 0xFF;
		info->model[i * 2 + 1] = buffer[27 + i] & 0xFF;
	}
	info->model[40] = '\0';

	/* 解析序列号 */
	for (int i = 0; i < 10; i++) {
		info->serial[i * 2] = (buffer[10 + i] >> 8) & 0xFF;
		info->serial[i * 2 + 1] = buffer[10 + i] & 0xFF;
	}
	info->serial[20] = '\0';

	/* 解析固件版本 */
	for (int i = 0; i < 4; i++) {
		info->firmware[i * 2] = (buffer[23 + i] >> 8) & 0xFF;
		info->firmware[i * 2 + 1] = buffer[23 + i] & 0xFF;
	}
	info->firmware[8] = '\0';

	/* 检查 LBA48 支持 */
	info->lba48_supported = (buffer[83] & 0x400) != 0;

	/* 获取容量 */
	if (info->lba48_supported)
		info->sectors = ((uint64_t)buffer[103] << 48) | ((uint64_t)buffer[102] << 32) |
						((uint64_t)buffer[101] << 16) | buffer[100];
	else
		info->sectors = ((uint32_t)buffer[61] << 16) | buffer[60];

	/* 获取能力信息 */
	info->capabilities = (buffer[49] << 16) | buffer[0];

	return ATA_SUCCESS;
}

/* 选择 ATA 驱动器 */
static ATA_RESULT ata_select_device(bool primary, bool master, uint32_t lba)
{
	uint16_t base = primary ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
	uint8_t dev = master ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
	/* 等待设备就绪 */

	ATA_RESULT err = ata_wait_ready(primary);
	if (err != ATA_SUCCESS)
		return err;

	/* 设置设备选择寄存器（LBA28 模式 + 主从选择 + LBA 24-27） */
	uint8_t dev_sel = 0xE0;			/* BIT6=1（LBA 模式），BIT7=1（强制选择） */
	dev_sel |= (dev & 0x01) << 4;	/* BIT4：0=主盘，1=从盘 */
	dev_sel |= (lba >> 24) & 0x0F;	/* BIT0-3：LBA24-LBA27（28 位 LBA 的高 4 位） */
	out8(base + ATA_REG_DRIVE_SELECT, dev_sel);

	/* 等待设备确认选择 */
	in8(base + ATA_REG_STATUS); /* 空读刷新状态 */

	/* 设置LBA低24位地址和扇区计数 */
	out8(base + ATA_REG_LBA_LOW, lba);			/* LBA 0-7 */
	out8(base + ATA_REG_LBA_MID, lba >> 8);		/* LBA 8-15 */
	out8(base + ATA_REG_LBA_HIGH, lba >> 16);	/* LBA 16-23 */
	out8(base + ATA_REG_SECTOR_COUNT, 1);		/* 默认 1 个扇区*/

	/* 再次等待设备就绪 */
	return ata_wait_ready(primary);
}

/*
	@brief ATA PIO 模式读取扇区。
	@param primary 是否为主通道
	@param master 是否为主盘
	@param lba 逻辑块地址
	@param sec_count 扇区计数
	@param buf 数据缓冲区指针
	@return ATA_RESULT 错误码
*/
ATA_RESULT ata_pio_read_sectors(bool primary, bool master, uint32_t lba, uint32_t sec_count, uint16_t *buf)
{
	uint16_t base = primary ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

	/* 检查参数合法性 */
	if (sec_count == 0 || buf == NULL) return ATA_ERR_INVALID_PARAM;

	/* 选择设备并设置 LBA 地址 */
	ATA_RESULT err = ata_select_device(primary, master, lba);
	if (err != ATA_SUCCESS) return err;

	/* 设置扇区计数 */
	out8(base + ATA_REG_SECTOR_COUNT, sec_count);

	/* 发送读扇区命令（PIO 模式） */
	out8(base + ATA_REG_COMMAND, ATA_CMD_READ_SECTOR);

	/* 读取每个扇区的数据 */
	for (uint32_t i = 0; i < sec_count; i++) {
		/* 等待当前扇区就绪 */
		err = ata_wait_drq(primary);
		if (err != ATA_SUCCESS) return ATA_ERR_READ_FAILED;

		/* 读取 512 字节 */
		for (int j = 0; j < SECTOR_SIZE / 2; j++)
			*buf++ = in16(base + ATA_REG_DATA);

		/* 空读状态寄存器 */
		in8(base + ATA_REG_STATUS);
	}

	return ATA_SUCCESS;
}

/*
	@brief ATA PIO 模式写入扇区。
	@param primary 是否为主通道
	@param master 是否为主盘
	@param lba 逻辑块地址
	@param sec_count 扇区计数
	@param buf 数据缓冲区指针
	@return ATA_RESULT 错误码
*/
ATA_RESULT ata_pio_write_sectors(bool primary, bool master, uint32_t lba, uint32_t sec_count, uint16_t *buf)
{
	uint16_t base = primary ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

	/* 检查参数合法性 */
	if (sec_count == 0 || buf == NULL) return ATA_ERR_INVALID_PARAM;

	/* 选择设备并设置 LBA 地址 */
	ATA_RESULT err = ata_select_device(primary, master, lba);
	if (err != ATA_SUCCESS) return err;

	/* 设置扇区计数 */
	out8(base + ATA_REG_SECTOR_COUNT, sec_count);

	/* 发送写扇区命令（PIO 模式） */
	out8(base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTOR);

	/* 读取每个扇区的数据 */
	for (uint32_t i = 0; i < sec_count; i++) {
		/* 等待当前扇区就绪 */
		err = ata_wait_drq(primary);
		if (err != ATA_SUCCESS) return ATA_ERR_WRITE_FAILED;

		/* 写入 512 字节 */
		for (int j = 0; j < SECTOR_SIZE / 2; j++)
			out16(base + ATA_REG_DATA, *buf++);

		/* 空读状态寄存器 */
		in8(base + ATA_REG_STATUS);
	}

	return ATA_SUCCESS;
}
