/*
	include/ClassiX/blkdev.h
*/

#ifndef _CLASSIX_BLKDEV_H_
#define _CLASSIX_BLKDEV_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

/* BIOS INT 13H 设备接口 */
#define BIOS_DEV_FD0						0x00	/* 第一张软盘 */
#define BIOS_DEV_FD1						0x01	/* 第二张软盘 */
#define BIOS_DEV_HD0						0x80	/* 第一张硬盘（主盘） */
#define BIOS_DEV_HD1						0x81	/* 第二张硬盘（从盘） */

typedef int32_t (*blkdev_read)(void *dev, uint32_t lba, uint32_t count, void *buf);
typedef int32_t (*blkdev_write)(void *dev, uint32_t lba, uint32_t count, const void *buf);

/* 块设备操作错误码 */
enum {
	BD_SUCCESS,				/* 成功 */
	BD_NOT_EXIST,			/* 设备不存在 */
	BD_INVALID_PARAM,		/* 无效参数 */
	BD_TIMEOUT,				/* 超时 */
	BD_NOT_READY,			/* 设备未就绪 */
	BD_IO_ERROR,			/* 读写错误 */
	BD_READONLY				/* 只读设备 */
};

/* 块设备 */
typedef struct {
	uint32_t id;			/* 设备 ID */
	uint32_t sector_size;	/* 扇区大小 */
	uint32_t total_sectors;	/* 总扇区数 */
	void *device;			/* 设备指针 */
	blkdev_read read;		/* 读扇区函数指针 */
	blkdev_write write;		/* 写扇区函数指针 */
} BLKDEV;

int32_t register_blkdevs(void);
BLKDEV *get_device(uint32_t dev_id);

#define HD_SECTOR_SIZE						512		/* 硬盘扇区大小 */

/* IDE 设备类型 */
enum {
	IDE_DEVICE_NONE,		/* 无设备 */
	IDE_DEVICE_HD,			/* 硬盘 */
	IDE_DEVICE_CDROM		/* CD-ROM */
};

/* IDE 设备 */
typedef struct {
	bool primary;			/* 是否为主通道 */
	bool master;			/* 是否为主盘 */
	uint8_t type;			/* 设备类型 */
	char model[41];			/* 型号 */
	char serial[21];		/* 序列号 */
	char firmware[9];		/* 固件版本 */
	uint64_t sectors;		/* 总扇区数 */
	uint32_t capabilities;	/* 能力信息 */
	bool lba48_supported;	/* 是否支持 LBA48 */
} IDE_DEVICE;

#define IDE_DEVICE_COUNT					4		/* IDE 设备数量 */

/* IDE 设备全局实例 */
extern IDE_DEVICE ide_devices[IDE_DEVICE_COUNT];

/* IDE 设备操作函数 */
uint32_t ide_init(void);
int32_t ide_read_sectors(IDE_DEVICE *dev, uint32_t lba, uint32_t sec_count, uint16_t *buf);
int32_t ide_write_sectors(IDE_DEVICE *dev, uint32_t lba, uint32_t sec_count, const uint16_t *buf);
int32_t ide_flush_cache(IDE_DEVICE *dev);

#define FD_SECTOR_SIZE						512		/* 软盘扇区大小 */

typedef struct {
	uint16_t cylinders;			/* 柱面数 */
	uint8_t heads;				/* 磁头数 */
	uint8_t sectors_per_track;	/* 每轨扇区数 */
	uint8_t gap3_length;		/* GAP3 长度 */
	uint8_t rate;				/* 传输速率 */
} FLOPPY_SPEC;

typedef struct {
	uint8_t drive;				/* 软盘驱动器号 */
	FLOPPY_SPEC *spec;			/* 软盘规格指针 */
} FD_DEVICE;

#define FD_DEVICE_COUNT						2		/* 软盘设备数量 */

/* 软盘设备全局实例 */
extern FD_DEVICE fd_devices[FD_DEVICE_COUNT];

/* 软盘设备操作函数 */
uint32_t fd_init(void);
int32_t fd_read_sectors(FD_DEVICE *dev, uint32_t lba, uint32_t sec_count, uint8_t *buf);
int32_t fd_write_sectors(FD_DEVICE *dev, uint32_t lba, uint32_t sec_count, const uint8_t *buf);

#ifdef __cplusplus
	}
#endif

#endif