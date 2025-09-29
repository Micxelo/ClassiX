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

typedef int32_t (*blkdev_io)(void *dev, uint32_t lba, uint32_t count, void *buf);

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
	blkdev_io read;			/* 读扇区函数指针 */
	blkdev_io write;		/* 写扇区函数指针 */
} BLKDEV;

int register_blkdevs(void);
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

/* IDE 设备数量 */
#define IDE_DEVICE_COUNT					4

/* IDE 设备全局实例 */
extern IDE_DEVICE ide_devices[IDE_DEVICE_COUNT];

/* IDE 设备操作函数 */
uint32_t ide_init(void);
int32_t ide_read_sectors(IDE_DEVICE *dev, uint32_t lba, uint32_t sec_count, uint16_t *buf);
int32_t ide_write_sectors(IDE_DEVICE *dev, uint32_t lba, uint32_t sec_count, const uint16_t *buf);
int32_t ide_flush_cache(IDE_DEVICE *dev);

#define FD_SECTOR_SIZE						512		/* 软盘扇区大小 */
#define FD_CYLINDERS						80		/* 柱面数 */
#define FD_HEADS							2		/* 磁头数 */
#define FD_SECTORS_PER_TRACK				18		/* 每磁道扇区数 */
#define FD_TOTAL_SECTORS					(FD_CYLINDERS * FD_HEADS * FD_SECTORS_PER_TRACK) /* 总扇区数 */

/* 软盘设备 */
typedef struct {
	uint8_t id;		/* 设备 ID */
	uint8_t type;	/* 驱动器类型 */
	bool present;	/* 是否有软盘 */
	bool motor;		/* 电机状态 */
} FD_DEVICE;

/* 软盘设备数量 */
#define FD_DEVICE_COUNT					4

/* 软盘设备全局实例 */
extern FD_DEVICE fd_devices[FD_DEVICE_COUNT];

/* 软盘设备操作函数 */
uint32_t fd_init(void);
int32_t fd_read_sectors(FD_DEVICE *dev, uint32_t lba, uint32_t count, void *buf);

#ifdef __cplusplus
	}
#endif

#endif