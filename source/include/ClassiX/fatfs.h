/*
	include/ClassiX/fatfs.h
*/

#ifndef _CLASSIX_FATFS_H_
#define _CLASSIX_FATFS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

/* MBR 分区表项 */
typedef struct __attribute__((packed)) {
	uint8_t bootable;		/* 引导标志 */
	uint8_t start_chs[3];	/* 起始 CHS（弃用） */
	uint8_t type;			/* 分区类型 */
	uint8_t end_chs[3];		/* 结束 CHS（弃用） */
	uint32_t start_lba;		/* 起始 LBA */
	uint32_t num_sectors;	/* 扇区数 */
} PART_ENTRY;

#define FAT_MAX_PATH						256

/* 仅支持 8.3 文件名 */
#define FAT_FILENAME_LEN					8	/* 文件名长度 */
#define FAT_EXT_LEN							3	/* 扩展名长度 */

/* FAT 类型枚举 */
typedef enum {
	FAT_TYPE_12,
	FAT_TYPE_16
} FAT_TYPE;

/* 通用目录项结构 */
typedef struct {
	char filename[FAT_EXT_LEN + 1];			/* 文件名 */
	char ext[FAT_EXT_LEN + 1];				/* 扩展名 */
	uint8_t attributes;						/* 文件属性 */
	uint16_t first_cluster;					/* 第一个所在簇 */
	size_t size;							/* 文件大小 */
} FAT_DIR_ENTRY;

/* 文件系统上下文 */
typedef struct {
	FAT_TYPE type;
	void *fs_ctx;
} FATFS;

/* 文件句柄 */
typedef struct {
	FATFS *fs;
	void *file_ctx;
	char path[FAT_MAX_PATH];
	bool is_open;
} FAT_FILE;

#ifdef __cplusplus
	}
#endif

#endif