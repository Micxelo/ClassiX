/*
	include/ClassiX/fatfs.h
*/

#ifndef _CLASSIX_FATFS_H_
#define _CLASSIX_FATFS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/blkdev.h>
#include <ClassiX/typedef.h>

#define MBR_SIGNATURE						0xaa55	/* MBR 标识 */

/* 分区类型 */
typedef enum {
	PART_TYPE_EMPTY = 0x00,
	PART_TYPE_FAT12 = 0x01,
	PART_TYPE_FAT16 = 0x04,
	PART_TYPE_FAT16B = 0x06,
	PART_TYPE_FAT16_LBA = 0x0e,
	PART_TYPE_FAT32 = 0x0b,
	PART_TYPE_FAT32_LBA = 0x0c,
	PART_TYPE_NTFS = 0x07,
	PART_TYPE_LINUX = 0x83,
	PART_TYPE_GPT = 0xee
} PART_TYPE;

/* MBR 分区表项 */
typedef struct __attribute__((packed)) {
	uint8_t bootable;					/* 引导标志 */
	uint8_t start_chs[3];				/* 起始 CHS（弃用） */
	uint8_t type;						/* 分区类型 */
	uint8_t end_chs[3];					/* 结束 CHS（弃用） */
	uint32_t start_lba;					/* 起始 LBA */
	uint32_t sector_count;				/* 扇区数 */
} PART_ENTRY;

typedef struct __attribute__((packed)) {
	uint8_t bootstrap[446];				/* 引导程序 */
	PART_ENTRY partitions[4];			/* 分区表 */
	uint16_t signature;					/* MBR 签名 */
} MBR;

#define FAT_MAX_PATH						256		/* 最大路径长度 */

/* 仅支持 8.3 文件名 */
#define FAT_FILENAME_LEN					8		/* 主文件名长度 */
#define FAT_EXT_LEN							3		/* 扩展名长度 */

/* 引导扇区结构 */
typedef struct __attribute__((packed)) {
	uint8_t jump_boot[3];				/* 跳转指令 */
	uint8_t oem_name[8];				/* 格式化该卷的 OEM 名称 */
	uint16_t bytes_per_sector;			/* 每扇区字节数 */
	uint8_t sectors_per_cluster;		/* 每簇扇区数 */
	uint16_t reserved_sectors;			/* 保留扇区数 */
	uint8_t fat_count;					/* FAT 表数量 */
	uint16_t root_entries;				/* 根目录条目数 */
	uint16_t total_sectors_short;		/* 总扇区数（当卷小于 32MB 时使用） */
	uint8_t media_descriptor;			/* 介质描述符，0xF8 表示固定磁盘 */
	uint16_t sectors_per_fat;			/* 每个 FAT 表占用的扇区数 */
	uint16_t sectors_per_track;			/* 每磁道扇区数 */
	uint16_t heads;						/* 磁头数 */
	uint32_t hidden_sectors;			/* 隐藏扇区数 */
	uint32_t total_sectors_long;		/* 总扇区数（当卷大于 32MB 时使用） */

	uint8_t drive_number;				/* 驱动器号 */
	uint8_t reserved;					/* 保留字节 */
	uint8_t boot_signature;				/* 扩展引导签名，0x29 表示有扩展信息 */
	uint32_t volume_id;					/* 卷序列号 */
	uint8_t volume_label[11];			/* 卷标 */
	uint8_t fs_type[8];					/* 文件系统类型 */
	uint8_t boot_code[448];				/* 引导代码*/
	uint16_t signature;					/* 引导扇区签名 */
} FAT_BPB;

/* 文件属性 */
typedef enum {
	FAT_ATTR_READONLY = 0x01,			/* 只读文件 */
	FAT_ATTR_HIDDEN = 0x02,				/* 隐藏文件 */
	FAT_ATTR_SYSTEM = 0x04,				/* 系统文件 */
	FAT_ATTR_VOLUME_ID = 0x08,			/* 卷标 */
	FAT_ATTR_DIRECTORY = 0x10,			/* 目录 */
	FAT_ATTR_ARCHIVE = 0x20,			/* 归档 */
	FAT_ATTR_LONGNAME = 0x0F			/* 长文件名 */
} FAT_ATTR;

/* 目录项结构 */
typedef struct __attribute__((packed)) {
	uint8_t filename[FAT_FILENAME_LEN];	/* 主文件名 */
	uint8_t extension[FAT_EXT_LEN];		/* 扩展名 */
	uint8_t attributes;					/* 文件属性 */
	uint8_t reserved;					/* 保留字节 */
	uint8_t creation_time_tenths;		/* 创建时间，单位 10ms */
	uint16_t creation_time;				/* 创建时间（时/分/秒） */
	uint16_t creation_date;				/* 创建日期（年/月/日） */
	uint16_t last_access_date;			/* 最后访问日期（年/月/日） */
	uint16_t first_cluster_high;		/* 起始簇号高 16 位（未使用） */
	uint16_t last_write_time;			/* 最后修改时间（时/分/秒） */
	uint16_t last_write_date;			/* 最后修改日期（年/月/日） */
	uint16_t first_cluster_low;			/* 起始簇号低 16 位 */
	uint32_t file_size;					/* 文件大小 */
} FAT_DIRENTRY;

/* FAT 类型枚举 */
typedef enum {
	FAT_TYPE_NONE,
	FAT_TYPE_12,
	FAT_TYPE_16
} FAT_TYPE;

/* 文件系统上下文 */
typedef struct {
	BLKDEV *device;							/* 文件系统所在物理块设备 */
	FAT_TYPE type;							/* FATFS 类型 */
	FAT_BPB bpb;							/* 分区引导扇区数据 */
	uint8_t *fat;							/* FAT 表数据 */
	size_t fat_size;						/* FAT 表大小 */
	uint32_t fs_sector;						/* 文件系统起始扇区 */
	uint32_t root_sector;					/* 根目录起始扇区 */
	uint32_t data_sector;					/* 数据起始扇区 */
} FATFS;

/* 文件句柄 */
typedef struct {
	FATFS *fs;								/* 文件系统上下文 */
	FAT_DIRENTRY *entry;						/* 文件目录项 */
	char path[FAT_MAX_PATH];				/* 文件路径 */
	bool is_open;							/* 是否打开 */
	uint32_t current_pos;					/* 当前读写位置 */
} FAT_FILE;

/* FATFS 错误码 */
enum {
	FATFS_SUCCESS = 0,						/* 成功 */
	FATFS_INVALID_PARAM,					/* 非法参数 */
	FATFS_READ_FAILED,						/* 读失败 */
	FATFS_WRITE_FAILED,						/* 写失败 */
	FATFS_INVALID_SIGNATURE,				/* 非法签名 */
	FATFS_INVALID_MBR,						/* 非法 MBR 参数 */
	FATFS_INVALID_BPB,						/* 非法 BPB 参数 */
	FATFS_NO_PARTITION,						/* 无指定分区 */
	FATFS_MEMORY_ALLOC,						/* 内存分配失败 */
	FATFS_NO_FREE_CLUSTER,					/* 无可用簇 */
	FATFS_ENTRY_EXISTS,						/* 条目已存在 */
	FATFS_ENTRY_NOT_FOUND,					/* 条目未找到 */
	FATFS_DIR_FULL,							/* 目录已满 */
	FATFS_IS_DIRECTORY,						/* 指定条目为目录 */
	FATFS_DIR_NOT_EMPTY						/* 目录不为空 */
};

void fatfs_convert_to_83(const char *filename, char *short_name, char *extension);
void fatfs_convert_from_83(const char *short_name, const char *extension, char *filename);
int32_t fatfs_init(FATFS *fs, BLKDEV *device, FAT_TYPE type);
int32_t fatfs_close(FATFS *fs);
int32_t fatfs_open_file(FAT_FILE *file, FATFS *fs, const char *path);
int32_t fatfs_create_file(FAT_FILE *file, FATFS *fs, const char *path);
int32_t fatfs_read_file(FAT_FILE *file, void *buffer, uint32_t size, uint32_t *bytes_read);
int32_t fatfs_write_file(FAT_FILE *file, const void *buffer, uint32_t size, bool append, uint32_t *bytes_written);
int32_t fatfs_close_file(FAT_FILE *file);
int32_t fatfs_delete_file(FATFS *fs, const char *path);
int32_t fatfs_create_directory(FATFS *fs, const char *path);
int32_t fatfs_delete_directory(FATFS *fs, const char *path);
int32_t fatfs_get_directory_entries(FATFS *fs, const char *path, FAT_DIRENTRY *entries, int32_t max_entries, int32_t *entries_read);
const char *fatfs_get_type_name(FAT_TYPE type);
void fatfs_get_attribute_names(uint8_t attributes, char *buffer, size_t size);
int32_t fatfs_get_fs_info(FATFS *fs, uint32_t *total_clusters, uint32_t *free_clusters, uint32_t *bytes_per_cluster);

#ifdef __cplusplus
	}
#endif

#endif