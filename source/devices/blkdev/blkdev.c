/*
	devices/blkdev/blkdev.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/typedef.h>

/* 全局块设备列表 */
static BLKDEV blkdev_list[16];
static int32_t blkdev_count = 0;

/* 硬盘读 */
static int32_t hd_read(void *dev, uint32_t lba, uint32_t count, void *buf)
{
	IDE_DEVICE *ide_dev = (IDE_DEVICE *) (((BLKDEV *) dev)->device);
	
	if (ide_dev->type == IDE_DEVICE_NONE)
		return BD_NOT_EXIST;
	
	return ide_read_sectors(ide_dev, lba, count, (uint16_t *) buf);
}

/* 硬盘写 */
static int32_t hd_write(void *dev, uint32_t lba, uint32_t count, void *buf)
{
	IDE_DEVICE *ide_dev = (IDE_DEVICE *) (((BLKDEV *) dev)->device);
	int32_t err;
	
	if (ide_dev->type == IDE_DEVICE_NONE)
		return BD_NOT_EXIST;
	
	err = ide_write_sectors(ide_dev, lba, count, (uint16_t *) buf);
	if (err != BD_SUCCESS)
		return err;
	
	/* 刷新缓存确保数据写入磁盘 */
	return ide_flush_cache(ide_dev);
}

/* 软盘读 (预留) */
static int32_t fd_read(void *dev, uint32_t lba, uint32_t count, void *buf)
{
	// TODO: 实现软盘读取
	return BD_NOT_READY;
}

/* 软盘写 (预留) */
static int32_t fd_write(void *dev, uint32_t lba, uint32_t count, void *buf)
{
	// TODO: 实现软盘写入
	return BD_NOT_READY;
}

/*
	@brief 注册块设备
	@param dev 块设备结构体指针
	@return 错误码
*/
static int32_t register_device(BLKDEV *dev)
{
	if (blkdev_count >= (signed) (sizeof(blkdev_list) / sizeof(blkdev_list[0])))
		return BD_INVALID_PARAM;
	
	/* 复制设备信息 */
	blkdev_list[blkdev_count] = *dev;
	blkdev_count++;
	
	return BD_SUCCESS;
}

/*
	@brief 注册块设备。
	@return 注册的设备数量
*/
int32_t register_blkdevs(void)
{
	int32_t count = 0;
	
	/* 初始化 IDE 设备 */
	if (ide_init() > 0) {
		/* 注册所有检测到的 IDE 设备 */
		for (int32_t i = 0; i < IDE_DEVICE_COUNT; i++) {
			if (ide_devices[i].type != IDE_DEVICE_NONE) {
				BLKDEV dev;
				
				/* 根据设备位置分配 BIOS 设备号 */
				uint32_t dev_id;
				if (ide_devices[i].primary)
					dev_id = ide_devices[i].master ? BIOS_DEV_HD0 : BIOS_DEV_HD1;
				else
					/* 从通道设备通常没有 BIOS 设备号 */
					dev_id = 0x82 + (ide_devices[i].master ? 0 : 1);
				
				dev.id = dev_id;
				dev.sector_size = HD_SECTOR_SIZE;
				dev.total_sectors = ide_devices[i].sectors;
				dev.read = hd_read;
				dev.write = hd_write;
				dev.device = &ide_devices[i]; /* 存储指向 IDE 设备的指针 */
				
				if (register_device(&dev) == BD_SUCCESS) {
					count++;
					debug("Registered IDE device: %s %s (BIOS: 0x%02X, %llu sectors).\n",
						  ide_devices[i].primary ? "primary" : "secondary",
						  ide_devices[i].master ? "master" : "slave",
						  dev_id, ide_devices[i].sectors);
				}
			}
		}
	}
	
	/* TODO: 未来可以在这里初始化软盘设备 */
	
	return count;
}

/*
	@brief 根据设备 ID 获取块设备。
	@param dev_id 设备 ID
	@return 块设备指针，如果不存在则返回 NULL
*/
BLKDEV *get_device(uint32_t dev_id)
{
	for (int32_t i = 0; i < blkdev_count; i++) {
		if (blkdev_list[i].id == dev_id)
			return &blkdev_list[i];
	}
	return NULL;
}
