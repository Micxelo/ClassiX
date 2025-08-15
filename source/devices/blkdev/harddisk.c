/*
	devices/blkdev/harddisk.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/io.h>
#include <ClassiX/typedef.h>

void ide_wait_ready(bool primary)
{
	uint16_t base = primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;
	uint8_t status;

	do {
		status = in8(base + IDE_REG_STATUS);
	} while (status & IDE_STATUS_BSY);

	if (status & (IDE_STATUS_DF | IDE_STATUS_ERR)) {
		/* 错误处理 */
		uint8_t error = in8(base + IDE_REG_ERROR);
		debug("IDE error: status=0x%02X, error=0x%02X.\n", status, error);
	}
}

int ide_identify(bool primary, bool master, IDE_DEVICE_INFO *info)
{
	uint16_t base = primary ? IDE_PRIMARY_BASE : IDE_SECONDARY_BASE;
	uint16_t buffer[256];

	/* 选择驱动器 */
	out8(base + IDE_REG_DRIVE_SELECT,
		 (master ? IDE_DRIVE_MASTER : IDE_DRIVE_SLAVE) | IDE_DRIVE_LBA_MASK);

	ide_wait_ready(primary);

	/* 发送 IDENTIFY 命令 */
	out8(base + IDE_REG_COMMAND, IDE_CMD_IDENTIFY);

	/* 等待设备响应 */
	uint8_t status = in8(base + IDE_REG_STATUS);
	if (!status)
		return -1; /* 设备不存在 */

	ide_wait_ready(primary);
	status = in8(base + IDE_REG_STATUS);

	if (status & IDE_STATUS_ERR) {
		/* 错误处理 */
		uint8_t error = in8(base + IDE_REG_ERROR);
		debug("IDENTIFY error: status=0x%02X, error=0x%02X.\n", status, error);
		return -1;
	}

	/* 读取数据 */
	for (int i = 0; i < 256; i++)
		buffer[i] = in16(base + IDE_REG_DATA);

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

	return 0;
}

void dma_init(uint32_t bm_base)
{
	/* 启用总线主控 */
	uint16_t command = in16(bm_base + BM_COMMAND_REG);
	out16(bm_base + BM_COMMAND_REG, command | 0x04);

	/* 清除状态寄存器 */
	out16(bm_base + BM_STATUS_REG, BM_STATUS_INTR | BM_STATUS_ERROR);
}

void dma_setup_prd(uint32_t bm_base, PRD *prd_table, uint32_t prd_phys)
{
	/* 停止当前 DMA 传输 */
	out8(bm_base + BM_COMMAND_REG, 0);

	/* 设置 PRD 表地址 */
	out32(bm_base + BM_PRDT_ADDR_REG, prd_phys);

	/* 清除状态寄存器 */
	out16(bm_base + BM_STATUS_REG, BM_STATUS_INTR | BM_STATUS_ERROR);
}
