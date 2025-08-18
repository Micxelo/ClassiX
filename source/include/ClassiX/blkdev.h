/*
	include/ClassiX/blkdev.h
*/

#ifndef _CLASSIX_BLKDEV_H_
#define _CLASSIX_BLKDEV_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define SECTOR_SIZE							512		/* 扇区大小 */

/* ATA 设备信息结构 */
typedef struct {
	char model[41];
	char serial[21];
	char firmware[9];
	uint64_t sectors;
	uint32_t capabilities;
	bool lba48_supported;
} ATA_DEVICE_INFO;

typedef enum {
	ATA_SUCCESS = 0,			/* 操作成功 */
	ATA_ERR_INVALID_PARAM,		/* 无效参数 */
	ATA_ERR_DEVICE_NOT_EXIT,	/* 设备不存在 */
	ATA_ERR_TIMEOUT,			/* 等待硬盘响应超时 */
	ATA_ERR_DEVICE_BUSY,		/* 设备持续忙 */
	ATA_ERR_DEVICE_NOT_READY,	/* 设备未就绪 */
	ATA_ERR_READ_FAILED,		/* 读扇区失败 */
	ATA_ERR_WRITE_FAILED,		/* 写扇区失败 */
	ATA_ERR_FLUSH_FAILED		/* 缓存刷新失败 */
} ATA_RESULT;

ATA_RESULT ata_identify(bool primary, bool master, ATA_DEVICE_INFO *info);
ATA_RESULT ata_pio_read_sectors(bool primary, bool master, uint32_t lba, uint32_t sec_count, uint16_t *buf);
ATA_RESULT ata_pio_write_sectors(bool primary, bool master, uint32_t lba, uint32_t sec_count, uint16_t *buf);

#ifdef __cplusplus
	}
#endif

#endif