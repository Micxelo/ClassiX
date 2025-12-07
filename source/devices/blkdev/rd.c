/*
	devices/blkdev/rd.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/typedef.h>

#include <string.h>

/*
	@brief RAM 磁盘读扇区。
	@param dev 目标 RAM 磁盘设备
	@param lba 起始扇区号
	@param count 读取扇区数量
	@param buf 数据缓冲区
	@return 错误码
*/
static int32_t rd_read(void *dev, uint32_t lba, uint32_t count, void *buf)
{
	uint8_t *rd_buf = (uint8_t *) ((BLKDEV *) dev)->device;
	uint32_t sector_size = ((BLKDEV *) dev)->sector_size;
	uint32_t total_sectors = ((BLKDEV *) dev)->total_sectors;

	if (lba + count > total_sectors)
		return BD_INVALID_PARAM;

	memcpy(buf, rd_buf + (lba * sector_size), count * sector_size);
	return BD_SUCCESS;
}

/*
	@brief RAM 磁盘写扇区。
	@param dev 目标 RAM 磁盘设备
	@param lba 起始扇区号
	@param count 写入扇区数量
	@param buf 数据缓冲区
	@return 错误码
*/
static int32_t rd_write(void *dev, uint32_t lba, uint32_t count, const void *buf)
{
	uint8_t *rd_buf = (uint8_t *) ((BLKDEV *) dev)->device;
	uint32_t sector_size = ((BLKDEV *) dev)->sector_size;
	uint32_t total_sectors = ((BLKDEV *) dev)->total_sectors;

	if (lba + count > total_sectors)
		return BD_INVALID_PARAM;

	memcpy(rd_buf + (lba * sector_size), buf, count * sector_size);
	return BD_SUCCESS;
}

/*
	@brief 初始化 RAM 磁盘设备。
	@param buf RAM 磁盘数据缓冲区
	@param bytes_per_sector 每扇区字节数
	@param total_sectors 总扇区数
	@param rd_dev 返回的块设备结构体指针
	@return 错误码
*/
int32_t rd_init(uint8_t *buf, uint32_t bytes_per_sector, uint32_t total_sectors, BLKDEV *rd_dev)
{
	if (rd_dev == NULL || buf == NULL || bytes_per_sector == 0 || total_sectors == 0)
		return BD_INVALID_PARAM;

	rd_dev->id = (uint32_t) rd_dev; /* 使用结构体指针作为设备 ID */
	rd_dev->sector_size = bytes_per_sector;
	rd_dev->total_sectors = total_sectors;
	rd_dev->device = (void*) buf;
	rd_dev->read = rd_read;
	rd_dev->write = rd_write;

	debug("RD: RAM Disk initialized: ID=0x%x, Sector size=%d, Total sectors=%d\n",
		rd_dev->id, rd_dev->sector_size, rd_dev->total_sectors);

	return BD_SUCCESS;
}
