/*
	utilities/fs/fat12.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/fatfs.h>
#include <ClassiX/memory.h>
#include <ClassiX/rtc.h>
#include <ClassiX/typedef.h>

#include <string.h>

#define ROOT_DIR_ENTRY_COUNT				224		/* 根目录项数 */
#define FAT_ENTRY_SIZE						3		/* 单个 FAT 项占用 1.5 字节 */

/*
	@brief 线程安全的字符串分割函数。
	@param str 待分割的字符串
	@param delim 分隔符字符串
	@param saveptr 保存上下文的指针
	@return 分割出的子字符串，若无更多子字符串则返回 NULL
*/
static char *strtok_r(char *str, const char *delim, char **saveptr)
{
	char *token;

	/* 使用 saveptr 中保存的位置 */
	if (str == NULL) {
		str = *saveptr;
		if (str == NULL)
			return NULL;
	}

	/* 跳过前导分隔符 */
	str += strspn(str, delim);

	/* 到达字符串末尾，返回 NULL */
	if (*str == '\0') {
		*saveptr = NULL;
		return NULL;
	}

	/* 找到 token 的结尾 */
	token = str;
	str = strpbrk(token, delim);

	if (str == NULL) {
		/* 最后一个 token */
		*saveptr = NULL;
	} else {
		/* 终止当前 token，保存下一个 token 的位置 */
		*str = '\0';
		*saveptr = str + 1;
	}

	return token;
}

/* 从设备读取一个扇区 */
static inline int32_t read_sector(const FATFS *fs, uint32_t sector, uint32_t count, uint8_t *buffer)
{
	return fs->device->read(fs->device, sector + fs->fs_sector, count, buffer);
}

/* 向设备写入一个扇区 */
static inline int32_t write_sector(const FATFS *fs, uint32_t sector, uint32_t count, const uint8_t *buffer)
{
	return fs->device->write(fs->device, sector + fs->fs_sector, count, buffer);
}

/* 计算的总簇数 */
static uint32_t calculate_total_clusters(const FAT_BPB *bpb)
{
	uint32_t total_sectors;
	uint32_t fat_sectors;
	uint32_t root_dir_sectors;
	uint32_t data_sectors;

	/* 确定总扇区数 */
	if (bpb->total_sectors_short != 0)
		total_sectors = bpb->total_sectors_short;
	else
		total_sectors = bpb->total_sectors_long;

	/* 计算 FAT 表占用的总扇区数 */
	fat_sectors = bpb->fat_count * bpb->sectors_per_fat;

	/* 计算根目录占用的扇区数 */
	root_dir_sectors = ((bpb->root_entries * sizeof(FAT_DIRENTRY)) + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector;

	/* 计算数据区扇区数 */
	data_sectors = total_sectors - (bpb->reserved_sectors + fat_sectors + root_dir_sectors);

	/* 计算总簇数 */
	return data_sectors / bpb->sectors_per_cluster;
}

/* 验证 FAT12 BPB 的合法性 */
static int32_t validate_fat12_bpb(const FAT_BPB *bpb)
{
	uint32_t total_clusters;
	bool valid_fs_type = false;

	/* 检查签名 */
	if (bpb->signature != MBR_SIGNATURE) {
		debug("FAT12: Invalid BPB signature: 0x%04x.\n", bpb->signature);
		return FATFS_INVALID_SIGNATURE;
	}

	/* 检查每扇区字节数 */
	if (bpb->bytes_per_sector != 512) {
		debug("FAT12: Unsupported bytes per sector: %u.\n", bpb->bytes_per_sector);
		return FATFS_INVALID_BPB;
	}

	/* 检查每簇扇区数 */
	if (bpb->sectors_per_cluster == 0 || (bpb->sectors_per_cluster & (bpb->sectors_per_cluster - 1)) != 0) {
		debug("FAT12: Invalid sectors per cluster: %u.\n", bpb->sectors_per_cluster);
		return FATFS_INVALID_BPB;
	}

	/* 检查 FAT 表数量 */
	if (bpb->fat_count == 0) {
		debug("FAT12: Invalid FAT count: %u.\n", bpb->fat_count);
		return FATFS_INVALID_BPB;
	}

	/* 检查根目录条目数 */
	if (bpb->root_entries == 0) {
		debug("FAT12: Invalid root entries: %u.\n", bpb->root_entries);
		return FATFS_INVALID_BPB;
	}

	/* 检查文件系统类型字符串 */
	const char fat12_fs_types[][9] = {"FAT12", "FAT12   ", "FAT", ""};
	for (int32_t i = 0; i < 4; i++) {
		if (strncmp(fat12_fs_types[i], (const char *) bpb->fs_type, strlen(fat12_fs_types[i])) == 0) {
			valid_fs_type = true;
			break;
		}
	}

	if (!valid_fs_type)
		debug("FAT12: Warning - unexpected filesystem type: \'%.8s\'.\n", bpb->fs_type);

	/* 计算总簇数并验证是否为 FAT12 */
	total_clusters = calculate_total_clusters(bpb);

	/* FAT12 的簇数应该小于 4085 */
	if (total_clusters >= 4085) {
		debug("FAT12: Not a FAT12 volume (total clusters: %u, expected < 4085).\n", total_clusters);
		return FATFS_INVALID_BPB;
	}

	debug("FAT12: Valid FAT12 BPB found - OEM: %.8s, FS type: %.8s, Clusters: %u.\n",
		bpb->oem_name, bpb->fs_type, total_clusters);

	return FATFS_SUCCESS;
}

/* 从 MBR 中查找 FAT12 分区 */
static int32_t find_fat12_partition(BLKDEV *device, uint32_t *partition_start)
{
	MBR mbr;
	int32_t i;

	/* 读取 MBR */
	if (device->read(device, 0, 1, (uint8_t *) &mbr) != BD_SUCCESS) {
		debug("FAT12: Failed to read MBR.\n");
		return FATFS_READ_FAILED;
	}

	/* 验证 MBR 签名 */
	if (mbr.signature != MBR_SIGNATURE) {
		debug("FAT12: Invalid MBR signature: 0x%04x.\n", mbr.signature);
		return FATFS_INVALID_MBR;
	}

	/* 遍历分区表查找 FAT12 分区 */
	for (i = 0; i < 4; i++) {
		if (mbr.partitions[i].type == PART_TYPE_FAT12 &&
			mbr.partitions[i].sector_count > 0) {
			*partition_start = mbr.partitions[i].start_lba;
			debug("FAT12: Found FAT12 partition at LBA %u.\n", *partition_start);
			return FATFS_SUCCESS;
		}
	}

	debug("FAT12: No FAT12 partition found in MBR.\n");
	return FATFS_NO_PARTITION;
}

/*
	@brief 初始化 FAT12 文件系统。
	@param fs 文件系统上下文
	@param device 文件系统所在物理块设备
	@param fs_sector 文件系统起始扇区
	@return 错误码
*/
int32_t fat12_init(FATFS *fs, BLKDEV *device)
{
	FAT_BPB bpb;
	uint32_t root_dir_sectors;
	uint32_t fat_size;
	uint32_t total_sectors;
	int32_t ret;

	/* 初始化文件系统上下文 */
	fs->device = device;
	fs->type = FAT_TYPE_12;
	fs->fat = NULL;
	fs->fs_sector = 0;

	debug("FAT12: Initializing FAT12 filesystem.\n");

	/* 尝试读取第 0 扇区作为 BPB */
	if (device->read(device, 0, 1, (uint8_t *) &bpb) != BD_SUCCESS) {
		debug("FAT12: Failed to read sector 0.\n");
		return FATFS_READ_FAILED;
	}

	/* 验证 BPB */
	ret = validate_fat12_bpb(&bpb);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Sector 0 is not a valid FAT12 BPB, checking for MBR.\n");

		/* 不是有效的 BPB，则查找 MBR 中的 FAT12 分区 */
		ret = find_fat12_partition(device, &fs->fs_sector);
		if (ret != FATFS_SUCCESS) {
			return ret;
		}

		/* 读取分区起始扇区的 BPB */
		if (device->read(device, fs->fs_sector, 1, (uint8_t *) &bpb) != BD_SUCCESS) {
			debug("FAT12: Failed to read partition BPB at LBA %u.\n", fs->fs_sector);
			return FATFS_READ_FAILED;
		}

		/* 再次验证 BPB */
		ret = validate_fat12_bpb(&bpb);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Partition BPB is invalid.\n");
			return ret;
		}
	} else {
		debug("FAT12: Using BPB from sector 0 (no MBR).\n");
	}

	/* 复制 BPB 数据 */
	fs->bpb = bpb;

	/* 计算总扇区数 */
	if (fs->bpb.total_sectors_short != 0)
		total_sectors = fs->bpb.total_sectors_short;
	else
		total_sectors = fs->bpb.total_sectors_long;

	debug("FAT12: Total sectors: %u, FAT count: %u, Sectors per FAT: %u.\n",
		total_sectors, fs->bpb.fat_count, fs->bpb.sectors_per_fat);

	/* 计算根目录起始扇区 */
	fs->root_sector = fs->bpb.reserved_sectors + (fs->bpb.fat_count * fs->bpb.sectors_per_fat);

	/* 计算根目录占用的扇区数 */
	root_dir_sectors = ((fs->bpb.root_entries * sizeof(FAT_DIRENTRY)) + fs->bpb.bytes_per_sector - 1) / fs->bpb.bytes_per_sector;

	/* 计算数据区起始扇区 */
	fs->data_sector = fs->root_sector + root_dir_sectors;

	debug("FAT12: Root sector: %u, Data sector: %u.\n", fs->root_sector, fs->data_sector);

	/* 分配 FAT 表内存 */
	fat_size = fs->bpb.sectors_per_fat * fs->bpb.bytes_per_sector;
	fs->fat = (uint8_t *) kmalloc(fat_size);
	if (fs->fat == NULL) {
		debug("FAT12: Failed to allocate memory for FAT table (%u bytes).\n", fat_size);
		return FATFS_MEMORY_ALLOC;
	}

	fs->fat_size = fat_size;

	/* 读取第一个 FAT 表 */
	if (read_sector(fs, fs->bpb.reserved_sectors, fs->bpb.sectors_per_fat, fs->fat) != BD_SUCCESS) {
		debug("FAT12: Failed to read FAT table.\n");
		kfree(fs->fat);
		fs->fat = NULL;
		return FATFS_READ_FAILED;
	}

	debug("FAT12: FAT12 filesystem initialized successfully.\n");
	debug("FAT12: Volume label: %.11s, FS type: %.8s.\n", fs->bpb.volume_label, fs->bpb.fs_type);

	return FATFS_SUCCESS;
}

/* 写回 FAT 表 */
static int32_t fat12_write_fat(FATFS *fs)
{
	int32_t ret = FATFS_SUCCESS;

	if (fs == NULL)
		return FATFS_INVALID_PARAM;

	/* 写回 FAT 表 */
	if (fs->fat != NULL) {
		/* FAT 表起始扇区 */
		uint32_t fat_start_sector = fs->bpb.reserved_sectors;

		/* 写回所有 FAT 表副本 */
		for (uint32_t i = 0; i < fs->bpb.fat_count; i++) {
			/* 计算当前 FAT 表的起始扇区 */
			uint32_t current_fat_sector = fat_start_sector + (i * fs->bpb.sectors_per_fat);

			debug("FAT12: Writing back FAT table %u at sector %u.\n", i, current_fat_sector);

			/* 写入当前 FAT 表 */
			if (write_sector(fs, current_fat_sector, fs->bpb.sectors_per_fat, fs->fat) != BD_SUCCESS) {
				debug("FAT12: Failed to write FAT table %u.\n", i);
				ret = FATFS_WRITE_FAILED;
				/* 继续尝试写回其他 FAT 表 */
			}
		}
	}

	return ret;
}

/*
	@brief 关闭 FAT12 文件系统。
	@param fs 文件系统上下文
	@return 错误码
*/
int32_t fat12_close(FATFS *fs)
{
	int32_t ret = FATFS_SUCCESS;

	if (fs == NULL)
		return FATFS_INVALID_PARAM;

	/* 写回 FAT 表 */
	if (fat12_write_fat(fs) == FATFS_SUCCESS) {
		/* 释放 FAT 表内存 */
		kfree(fs->fat);
		fs->fat = NULL;
		fs->fat_size = 0;
	}

	/* 清除文件系统上下文 */
	fs->device = NULL;
	fs->type = FAT_TYPE_NONE;
	fs->fs_sector = 0;
	fs->root_sector = 0;
	fs->data_sector = 0;

	/* 清零 BPB */
	memset(&fs->bpb, 0, sizeof(FAT_BPB));

	if (ret == FATFS_SUCCESS)
		debug("FAT12: Filesystem closed successfully.\n");
	else
		debug("FAT12: Filesystem closed with errors.\n");

	return ret;
}

/*
	@brief 从 FAT 表中获取下一个簇号。
	@param fs 文件系统上下文
	@param cluster 当前簇号
	@param next_cluster 返回的下一个簇号
	@return 错误码
*/
int32_t fat12_get_next_cluster(FATFS *fs, uint16_t cluster, uint16_t *next_cluster)
{
	uint32_t fat_offset;
	uint16_t value;

	/* 参数检查 */
	if (fs == NULL || next_cluster == NULL) {
		debug("FAT12: Invalid parameters in fat12_get_next_cluster.\n");
		return FATFS_INVALID_PARAM;
	}

	if (fs->fat == NULL) {
		debug("FAT12: FAT table not loaded.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 簇号有效性检查 */
	if (cluster < 2) {
		debug("FAT12: Invalid cluster number: %u.\n", cluster);
		return FATFS_INVALID_PARAM;
	}

	/* 计算 FAT 表中的偏移量 */
	fat_offset = cluster * 3 / 2;

	/* 检查偏移量是否超出 FAT 表边界 */
	if (fat_offset + 1 >= fs->fat_size) {
		debug("FAT12: Cluster %u exceeds FAT table boundaries.\n", cluster);
		return FATFS_INVALID_PARAM;
	}

	/* 根据簇号的奇偶性读取 12 位 FAT 项 */
	if (cluster % 2 == 0)
		/* 偶数簇号：取低 12 位 */
		value = fs->fat[fat_offset] | ((fs->fat[fat_offset + 1] & 0x0F) << 8);
	else
		/* 奇数簇号：取高 12 位 */
		value = ((fs->fat[fat_offset] & 0xF0) >> 4) | (fs->fat[fat_offset + 1] << 4);

	/* 不超过 12 位 */
	value &= 0x0FFF;

	*next_cluster = value;

	debug("FAT12: Next cluster for %u is %u.\n", cluster, value);

	return FATFS_SUCCESS;
}

/*
	@brief 设置 FAT 表中的下一个簇号。
	@param fs 文件系统上下文
	@param cluster 当前簇号
	@param value 要设置的下一个簇号值
	@return 错误码
*/
static int32_t fat12_set_next_cluster(FATFS *fs, uint16_t cluster, uint16_t value)
{
	uint32_t fat_offset;

	/* 参数检查 */
	if (fs == NULL) {
		debug("FAT12: Invalid parameters in set_next_cluster.\n");
		return FATFS_INVALID_PARAM;
	}

	if (fs->fat == NULL) {
		debug("FAT12: FAT table not loaded.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 簇号有效性检查 */
	if (cluster < 2) {
		debug("FAT12: Invalid cluster number: %u.\n", cluster);
		return FATFS_INVALID_PARAM;
	}

	/* 值有效性检查 */
	if (value > 0x0FFF) {
		debug("FAT12: Invalid next cluster value: 0x%03X.\n", value);
		return FATFS_INVALID_PARAM;
	}

	/* 计算 FAT 表中的偏移量 */
	fat_offset = cluster * 3 / 2;

	/* 检查偏移量是否超出 FAT 表边界 */
	if (fat_offset + 1 >= fs->fat_size) {
		debug("FAT12: Cluster %u exceeds FAT table boundaries.\n", cluster);
		return FATFS_INVALID_PARAM;
	}

	/* 根据簇号的奇偶性设置 12 位 FAT 项 */
	if (cluster % 2 == 0) {
		/* 偶数簇号：设置低 12 位 */
		fs->fat[fat_offset] = value & 0xFF;
		fs->fat[fat_offset + 1] = (fs->fat[fat_offset + 1] & 0xF0) | ((value >> 8) & 0x0F);
	} else {
		/* 奇数簇号：设置高 12 位 */
		fs->fat[fat_offset] = (fs->fat[fat_offset] & 0x0F) | ((value << 4) & 0xF0);
		fs->fat[fat_offset + 1] = (value >> 4) & 0xFF;
	}

	debug("FAT12: Set next cluster for %u to %u.\n", cluster, value);

	/* 写回 FAT 表 */
	return fat12_write_fat(fs);
}

/*
	@brief 分配一个新的 FAT 簇。
	@param fs 文件系统上下文
	@param allocated_cluster 返回分配的簇号
	@return 错误码
*/
static int32_t fat12_allocate_cluster(FATFS *fs, uint16_t *allocated_cluster)
{
	uint16_t cluster;
	uint16_t next_cluster;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || allocated_cluster == NULL) {
		debug("FAT12: Invalid parameters in allocate_cluster.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 计算最大簇号 */
	uint32_t total_clusters = calculate_total_clusters(&fs->bpb);
	uint16_t max_cluster = (total_clusters + 1) & 0x0FFF; /* 不超过 12 位 */

	if (max_cluster > 0xFF5)
		max_cluster = 0xFF5; /* FAT12 最大簇号为 4085 (0xFF5) */

	debug("FAT12: Searching for free cluster from 2 to %u (0x%03X).\n", max_cluster, max_cluster);

	/* 遍历簇号查找空闲簇 */
	for (cluster = 2; cluster <= max_cluster; cluster++) {
		/* 获取当前簇的下一个簇号 */
		ret = fat12_get_next_cluster(fs, cluster, &next_cluster);
		if (ret != FATFS_SUCCESS)
			/* 跳过错误簇，继续查找 */
			continue;

		/* 检查是否为空闲簇 */
		if (next_cluster == 0x000) {
			/* 找到空闲簇，标记为已使用 */
			ret = fat12_set_next_cluster(fs, cluster, 0xFFF);
			if (ret == FATFS_SUCCESS) {
				*allocated_cluster = cluster;
				debug("FAT12: Allocated cluster %u (0x%03X).\n", cluster, cluster);
				return FATFS_SUCCESS;
			} else {
				debug("FAT12: Failed to mark cluster %u as allocated.\n", cluster);
				return ret;
			}
		}
	}

	/* 没有找到可用簇 */
	debug("FAT12: No free clusters available.\n");
	*allocated_cluster = 0;
	return FATFS_NO_FREE_CLUSTER;
}

/*
	@brief 释放 FAT 簇链。
	@param fs 文件系统上下文
	@param start_cluster 簇链的起始簇号
	@return 错误码
*/
static int32_t fat12_free_clusters(FATFS *fs, uint16_t start_cluster)
{
	uint16_t current_cluster = start_cluster;
	uint16_t next_cluster;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL) {
		debug("FAT12: Invalid parameters in free_clusters.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 起始簇号有效性检查 */
	if (start_cluster < 2) {
		debug("FAT12: Invalid start cluster: %u.\n", start_cluster);
		return FATFS_INVALID_PARAM;
	}

	debug("FAT12: Freeing cluster chain starting at %u.\n", start_cluster);

	/* 遍历簇链并释放每个簇 */
	while (current_cluster >= 2 && current_cluster < 0xFF8) {
		/* 获取下一个簇号 */
		ret = fat12_get_next_cluster(fs, current_cluster, &next_cluster);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to get next cluster for %u.\n", current_cluster);
			return ret;
		}

		/* 释放当前簇 */
		ret = fat12_set_next_cluster(fs, current_cluster, 0x000);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to free cluster %u.\n", current_cluster);
			return ret;
		}

		debug("FAT12: Freed cluster %u.\n", current_cluster);
		current_cluster = next_cluster;
	}

	debug("FAT12: Successfully freed cluster chain.\n");
	return FATFS_SUCCESS;
}

/*
	@brief 读取指定簇的数据。
	@param fs 文件系统上下文
	@param cluster 要读取的簇号
	@param buffer 数据缓冲区
	@return 错误码
*/
static int32_t fat12_read_cluster(FATFS *fs, uint16_t cluster, uint8_t *buffer)
{
	uint32_t sector;
	uint32_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint32_t i;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || buffer == NULL) {
		debug("FAT12: Invalid parameters in read_cluster.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 簇号有效性检查 */
	if (cluster < 2 || cluster >= 0xFF8) {
		debug("FAT12: Invalid cluster number: %u.\n", cluster);
		return FATFS_INVALID_PARAM;
	}

	bytes_per_sector = fs->bpb.bytes_per_sector;
	sectors_per_cluster = fs->bpb.sectors_per_cluster;

	/* 计算簇对应的起始扇区 */
	sector = fs->data_sector + (cluster - 2) * sectors_per_cluster;

	debug("FAT12: Reading cluster %u from sector %u.\n", cluster, sector);

	/* 读取簇的所有扇区 */
	for (i = 0; i < sectors_per_cluster; i++) {
		ret = read_sector(fs, sector + i, 1, buffer + i * bytes_per_sector);
		if (ret != BD_SUCCESS) {
			debug("FAT12: Failed to read sector %u for cluster %u.\n", sector + i, cluster);
			return FATFS_READ_FAILED;
		}
	}

	debug("FAT12: Successfully read cluster %u.\n", cluster);
	return FATFS_SUCCESS;
}

/*
	@brief 写入数据到指定簇。
	@param fs 文件系统上下文
	@param cluster 要写入的簇号
	@param buffer 数据缓冲区
	@return 错误码
*/
static int32_t fat12_write_cluster(FATFS *fs, uint16_t cluster, const uint8_t *buffer)
{
	uint32_t sector;
	uint32_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint32_t i;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || buffer == NULL) {
		debug("FAT12: Invalid parameters in write_cluster.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 簇号有效性检查 */
	if (cluster < 2 || cluster >= 0xFF8) {
		debug("FAT12: Invalid cluster number: %u.\n", cluster);
		return FATFS_INVALID_PARAM;
	}

	bytes_per_sector = fs->bpb.bytes_per_sector;
	sectors_per_cluster = fs->bpb.sectors_per_cluster;

	/* 计算簇对应的起始扇区 */
	sector = fs->data_sector + (cluster - 2) * sectors_per_cluster;

	debug("FAT12: Writing cluster %u to sector %u.\n", cluster, sector);

	/* 写入簇的所有扇区 */
	for (i = 0; i < sectors_per_cluster; i++) {
		ret = write_sector(fs, sector + i, 1, buffer + i * bytes_per_sector);
		if (ret != BD_SUCCESS) {
			debug("FAT12: Failed to write sector %u for cluster %u.\n", sector + i, cluster);
			return FATFS_WRITE_FAILED;
		}
	}

	debug("FAT12: Successfully wrote cluster %u.\n", cluster);
	return FATFS_SUCCESS;
}

/*
	@brief 读取目录项。
	@param fs 文件系统上下文
	@param dir_cluster 目录簇号
	@param entry_num 目录项序号
	@param entry 返回的目录项
	@return 错误码
*/
static int32_t fat12_read_directory_entry(FATFS *fs, uint16_t dir_cluster, int32_t entry_num, FAT_DIRENTRY *entry)
{
	uint8_t buffer[512];
	int32_t entries_per_sector = fs->bpb.bytes_per_sector / sizeof(FAT_DIRENTRY);
	int32_t sector_num, sector_offset;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || entry == NULL) {
		debug("FAT12: Invalid parameters in read_directory_entry.\n");
		return FATFS_INVALID_PARAM;
	}

	if (dir_cluster == 0) {
		/* 根目录 */
		if (entry_num >= fs->bpb.root_entries) {
			debug("FAT12: Directory entry %d out of root directory bounds.\n", entry_num);
			return FATFS_INVALID_PARAM;
		}

		sector_num = fs->root_sector + (entry_num / entries_per_sector);
		sector_offset = entry_num % entries_per_sector;
	} else {
		/* 子目录 */
		int32_t entries_per_cluster = (fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster) / sizeof(FAT_DIRENTRY);
		int32_t cluster_num = entry_num / entries_per_cluster;
		int32_t cluster_offset = entry_num % entries_per_cluster;
		uint16_t current_cluster = dir_cluster;

		/* 遍历簇链找到目标簇 */
		for (int32_t i = 0; i < cluster_num; i++) {
			uint16_t next_cluster;
			ret = fat12_get_next_cluster(fs, current_cluster, &next_cluster);
			if (ret != FATFS_SUCCESS)
				return ret;

			if (next_cluster >= 0xFF8) {
				/* 未找到目标簇 */
				debug("FAT12: End of cluster chain reached while searching for entry %d.\n", entry_num);
				return FATFS_READ_FAILED;
			}
			current_cluster = next_cluster;
		}

		/* 计算目标扇区 */
		uint32_t data_sector = fs->data_sector + (current_cluster - 2) * fs->bpb.sectors_per_cluster;
		sector_num = data_sector + (cluster_offset / entries_per_sector);
		sector_offset = cluster_offset % entries_per_sector;
	}

	/* 读取扇区并获取目录项 */
	ret = read_sector(fs, sector_num, 1, buffer);
	if (ret != BD_SUCCESS) {
		debug("FAT12: Failed to read sector %d for directory entry.\n", sector_num);
		return FATFS_READ_FAILED;
	}

	memcpy(entry, &buffer[sector_offset * sizeof(FAT_DIRENTRY)], sizeof(FAT_DIRENTRY));

	debug("FAT12: Read directory entry %d from sector %d, offset %d.\n", entry_num, sector_num, sector_offset);

	return FATFS_SUCCESS;
}

/*
	@brief 写入目录项。
	@param fs 文件系统上下文
	@param dir_cluster 目录簇号
	@param entry_num 目录项序号
	@param entry 要写入的目录项
	@return 错误码
*/
static int32_t fat12_write_directory_entry(FATFS *fs, uint16_t dir_cluster, int32_t entry_num, const FAT_DIRENTRY *entry)
{
	uint8_t buffer[512];
	int32_t entries_per_sector = fs->bpb.bytes_per_sector / sizeof(FAT_DIRENTRY);
	int32_t sector_num, sector_offset;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || entry == NULL) {
		debug("FAT12: Invalid parameters in write_directory_entry.\n");
		return FATFS_INVALID_PARAM;
	}

	if (dir_cluster == 0) {
		/* 根目录 */
		if (entry_num >= fs->bpb.root_entries) {
			debug("FAT12: Directory entry %d out of root directory bounds.\n", entry_num);
			return FATFS_INVALID_PARAM;
		}

		sector_num = fs->root_sector + (entry_num / entries_per_sector);
		sector_offset = entry_num % entries_per_sector;
	} else {
		/* 子目录 */
		int32_t entries_per_cluster = (fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster) / sizeof(FAT_DIRENTRY);
		int32_t cluster_num = entry_num / entries_per_cluster;
		int32_t cluster_offset = entry_num % entries_per_cluster;
		uint16_t current_cluster = dir_cluster;
		uint16_t next_cluster;

		/* 遍历簇链找到目标簇 */
		for (int32_t i = 0; i < cluster_num; i++) {
			ret = fat12_get_next_cluster(fs, current_cluster, &next_cluster);
			if (ret != FATFS_SUCCESS)
				return ret;

			if (next_cluster >= 0xFF8) {
				/* 未找到目标簇，分配新簇 */
				uint16_t new_cluster;
				ret = fat12_allocate_cluster(fs, &new_cluster);
				if (ret != FATFS_SUCCESS) {
					debug("FAT12: Failed to allocate new cluster for directory expansion.\n");
					return ret;
				}

				/* 设置当前簇指向新簇，新簇标记为结束 */
				ret = fat12_set_next_cluster(fs, current_cluster, new_cluster);
				if (ret != FATFS_SUCCESS) {
					debug("FAT12: Failed to set next cluster for directory expansion.\n");
					return ret;
				}

				ret = fat12_set_next_cluster(fs, new_cluster, 0xFFF);
				if (ret != FATFS_SUCCESS) {
					debug("FAT12: Failed to mark new cluster as end of chain.\n");
					return ret;
				}

				current_cluster = new_cluster;
			} else {
				current_cluster = next_cluster;
			}
		}

		/* 计算目标扇区 */
		uint32_t data_sector = fs->data_sector + (current_cluster - 2) * fs->bpb.sectors_per_cluster;
		sector_num = data_sector + (cluster_offset / entries_per_sector);
		sector_offset = cluster_offset % entries_per_sector;
	}

	/* 更新目录项 */
	ret = read_sector(fs, sector_num, 1, buffer);
	if (ret != BD_SUCCESS) {
		debug("FAT12: Failed to read sector %d for directory entry update.\n", sector_num);
		return FATFS_READ_FAILED;
	}

	memcpy(&buffer[sector_offset * sizeof(FAT_DIRENTRY)], entry, sizeof(FAT_DIRENTRY));

	ret = write_sector(fs, sector_num, 1, buffer);
	if (ret != BD_SUCCESS) {
		debug("FAT12: Failed to write sector %d with updated directory entry.\n", sector_num);
		return FATFS_WRITE_FAILED;
	}

	debug("FAT12: Wrote directory entry %d to sector %d, offset %d.\n", entry_num, sector_num, sector_offset);

	return FATFS_SUCCESS;
}

/*
	@brief 在目录中查找指定文件名的目录项。
	@param fs 文件系统上下文
	@param dir_cluster 目录簇号
	@param filename 要查找的文件名
	@param entry 返回找到的目录项
	@param entry_index 返回目录项索引
	@return 错误码
*/
static int32_t fat12_find_entry(FATFS *fs, uint16_t dir_cluster, const char *filename, FAT_DIRENTRY *entry, int32_t *entry_index)
{
	char short_name[FAT_FILENAME_LEN + 1];
	char ext[FAT_EXT_LEN + 1];
	int32_t max_entries;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || filename == NULL) {
		debug("FAT12: Invalid parameters in find_entry.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 转换为 8.3 格式 */
	fatfs_convert_to_83(filename, short_name, ext);

	/* 确定最大条目数 */
	if (dir_cluster == 0) {
		/* 根目录 */
		max_entries = fs->bpb.root_entries;
	} else {
		/* 子目录 - 遍历簇链计算总条目数 */
		max_entries = 0;
		uint16_t current_cluster = dir_cluster;
		uint16_t next_cluster;

		while (current_cluster >= 2 && current_cluster < 0xFF8) {
			max_entries += (fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster) / sizeof(FAT_DIRENTRY);

			ret = fat12_get_next_cluster(fs, current_cluster, &next_cluster);
			if (ret != FATFS_SUCCESS)
				break;

			current_cluster = next_cluster;
		}
	}

	debug("FAT12: Searching for \'%.8s.%.3s\' in directory cluster %u (max entries: %d).\n",
		short_name, ext, dir_cluster, max_entries);

	/* 搜索目录项 */
	for (int32_t i = 0; i < max_entries; i++) {
		FAT_DIRENTRY temp_entry;

		ret = fat12_read_directory_entry(fs, dir_cluster, i, &temp_entry);
		if (ret != FATFS_SUCCESS)
			continue;

		/* 检查是否为已删除条目 */
		if (temp_entry.filename[0] == 0xE5)
			continue;

		/* 检查是否为空闲条目（目录结束） */
		if (temp_entry.filename[0] == 0x00)
			break;

		/* 比较文件名和扩展名 */
		if (memcmp(temp_entry.filename, short_name, FAT_FILENAME_LEN) == 0 &&
			memcmp(temp_entry.extension, ext, FAT_EXT_LEN) == 0) {

			if (entry != NULL)
				memcpy(entry, &temp_entry, sizeof(FAT_DIRENTRY));

			if (entry_index != NULL)
				*entry_index = i;

			debug("FAT12: Found entry at index %d.\n", i);
			return FATFS_SUCCESS;
		}
	}

	debug("FAT12: Entry not found.\n");
	return FATFS_ENTRY_NOT_FOUND;
}

/*
	@brief 创建新的目录项。
	@param fs 文件系统上下文
	@param dir_cluster 目录簇号
	@param filename 文件名
	@param attributes 文件属性
	@param start_cluster 起始簇号
	@param file_size 文件大小
	@param new_entry 返回新创建的目录项
	@return 错误码
*/
static int32_t fat12_create_entry(FATFS *fs, uint16_t dir_cluster, const char *filename, uint8_t attributes, uint16_t start_cluster, uint32_t file_size, FAT_DIRENTRY *new_entry)
{
	char short_name[FAT_FILENAME_LEN + 1];
	char ext[FAT_EXT_LEN + 1];
	FAT_DIRENTRY entry;
	int32_t free_entry_index = -1;
	int32_t max_entries;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || filename == NULL) {
		debug("FAT12: Invalid parameters in create_entry.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 转换为 8.3 格式 */
	fatfs_convert_to_83(filename, short_name, ext);

	/* 检查是否已存在同名条目 */
	ret = fat12_find_entry(fs, dir_cluster, filename, NULL, NULL);
	if (ret == FATFS_SUCCESS) {
		debug("FAT12: Entry \'%.8s.%.3s\' already exists.\n", short_name, ext);
		return FATFS_ENTRY_EXISTS;
	}

	/* 确定最大条目数 */
	if (dir_cluster == 0) {
		/* 根目录 */
		max_entries = fs->bpb.root_entries;
	} else {
		/* 子目录 - 遍历簇链计算总条目数 */
		max_entries = 0;
		uint16_t current_cluster = dir_cluster;
		uint16_t next_cluster;

		while (current_cluster >= 2 && current_cluster < 0xFF8) {
			max_entries += (fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster) / sizeof(FAT_DIRENTRY);

			ret = fat12_get_next_cluster(fs, current_cluster, &next_cluster);
			if (ret != FATFS_SUCCESS)
				break;

			current_cluster = next_cluster;
		}
	}

	debug("FAT12: Looking for free entry in directory cluster %u (max entries: %d).\n",
		dir_cluster, max_entries);

	/* 查找空闲条目 */
	for (int32_t i = 0; i < max_entries; i++) {
		ret = fat12_read_directory_entry(fs, dir_cluster, i, &entry);
		if (ret != FATFS_SUCCESS)
			continue;

		/* 检查是否为已删除条目或空闲条目 */
		if (entry.filename[0] == 0xE5 || entry.filename[0] == 0x00) {
			free_entry_index = i;
			break;
		}
	}

	/* 没有找到空闲条目，返回错误 */
	if (free_entry_index == -1) {
		debug("FAT12: No free entries available in directory.\n");
		return FATFS_DIR_FULL;
	}

	/* 初始化新目录项 */
	memset(&entry, 0, sizeof(FAT_DIRENTRY));
	memcpy(entry.filename, short_name, FAT_FILENAME_LEN);
	memcpy(entry.extension, ext, FAT_EXT_LEN);
	entry.attributes = attributes;

	/* 设置时间和日期为当前时间 */
	uint32_t year = get_year();
	uint32_t month = get_month();
	uint32_t day = get_day_of_month();
	uint32_t hour = get_hour();
	uint32_t minute = get_minute();
	uint32_t second = get_second();

	/* 转换为 FAT 时间格式 */
	uint16_t fat_time = ((hour & 0x1F) << 11) | ((minute & 0x3F) << 5) | ((second / 2) & 0x1F);
	uint16_t fat_date = (((year - 1980) & 0x7F) << 9) | ((month & 0x0F) << 5) | (day & 0x1F);

	/* 设置 FAT 时间和日期 */
	entry.creation_time_tenths = (second % 2) * 100; /* 10ms 精度 */
	entry.creation_time = fat_time;
	entry.creation_date = fat_date;
	entry.last_write_time = fat_time;
	entry.last_write_date = fat_date;
	entry.last_access_date = fat_date;

	entry.first_cluster_low = start_cluster;
	entry.file_size = file_size;

	/* 写入新目录项 */
	ret = fat12_write_directory_entry(fs, dir_cluster, free_entry_index, &entry);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to write new directory entry.\n");
		return ret;
	}

	/* 返回新创建的目录项 */
	if (new_entry != NULL)
		memcpy(new_entry, &entry, sizeof(FAT_DIRENTRY));

	debug("FAT12: Created new entry \'%.8s.%.3s\' at index %d.\n", short_name, ext, free_entry_index);

	return FATFS_SUCCESS;
}

/*
	@brief 删除目录项。
	@param fs 文件系统上下文
	@param dir_cluster 目录簇号（0表示根目录）
	@param filename 要删除的文件名
	@return 错误码
*/
static int32_t fat12_delete_entry(FATFS *fs, uint16_t dir_cluster, const char *filename)
{
	FAT_DIRENTRY entry;
	int32_t entry_index;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || filename == NULL) {
		debug("FAT12: Invalid parameters in delete_entry.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 查找目录项 */
	ret = fat12_find_entry(fs, dir_cluster, filename, &entry, &entry_index);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Entry not found for deletion.\n");
		return FATFS_ENTRY_NOT_FOUND;
	}

	/* 标记为已删除 */
	entry.filename[0] = 0xE5;

	/* 写回修改后的目录项 */
	ret = fat12_write_directory_entry(fs, dir_cluster, entry_index, &entry);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to mark entry as deleted.\n");
		return ret;
	}

	/* 释放文件或目录的簇链 */
	if (!(entry.attributes & FAT_ATTR_VOLUME_ID) && entry.first_cluster_low != 0) {
		ret = fat12_free_clusters(fs, entry.first_cluster_low);
		if (ret != FATFS_SUCCESS)
			debug("FAT12: Failed to free clusters for deleted entry.\n");
	}

	debug("FAT12: Deleted entry \'%s\' at index %d.\n", filename, entry_index);

	return FATFS_SUCCESS;
}

/*
	@brief 读取目录内容。
	@param fs 文件系统上下文
	@param dir_cluster 目录簇号
	@param entries 目录项数组
	@param max_entries 最大目录项数
	@param count 保存实际读取的目录项数
	@return 错误码
*/
static int32_t fat12_read_directory(FATFS *fs, uint16_t dir_cluster, FAT_DIRENTRY *entries, int32_t max_entries, int32_t *count)
{
	int32_t num = 0;
	int32_t max_dir_entries;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || entries == NULL || max_entries <= 0) {
		debug("FAT12: Invalid parameters in read_directory.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 确定最大条目数 */
	if (dir_cluster == 0) {
		/* 根目录 */
		max_dir_entries = fs->bpb.root_entries;
	} else {
		/* 子目录 - 遍历簇链计算总条目数 */
		max_dir_entries = 0;
		uint16_t current_cluster = dir_cluster;
		uint16_t next_cluster;

		while (current_cluster >= 2 && current_cluster < 0xFF8) {
			max_dir_entries += (fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster) / sizeof(FAT_DIRENTRY);

			ret = fat12_get_next_cluster(fs, current_cluster, &next_cluster);
			if (ret != FATFS_SUCCESS)
				break;

			current_cluster = next_cluster;
		}
	}

	debug("FAT12: Reading directory cluster %u (max entries: %d).\n", dir_cluster, max_dir_entries);

	/* 读取目录项 */
	for (int32_t i = 0; i < max_dir_entries && num < max_entries; i++) {
		FAT_DIRENTRY entry;

		ret = fat12_read_directory_entry(fs, dir_cluster, i, &entry);
		if (ret != FATFS_SUCCESS)
			continue;

		/* 检查是否为已删除条目 */
		if (entry.filename[0] == 0xE5)
			continue;

		/* 检查是否为空闲条目，到达目录末尾 */
		if (entry.filename[0] == 0x00)
			break;

		/* 跳过长文件名条目 */
		if ((entry.attributes & FAT_ATTR_LONGNAME) == FAT_ATTR_LONGNAME)
			continue;

		/* 跳过卷标条目 */
		if (entry.attributes & FAT_ATTR_VOLUME_ID)
			continue;

		/* 复制目录项 */
		memcpy(&entries[num], &entry, sizeof(FAT_DIRENTRY));
		num++;
	}

	debug("FAT12: Read %d entries from directory cluster %u.\n", num, dir_cluster);
	*count = num;
	return FATFS_SUCCESS;
}

/*
	@brief 解析路径，获取父目录和目标名称。
	@param fs 文件系统上下文
	@param path 路径字符串
	@param parent_dir 返回父目录的目录项
	@param filename 返回目标文件名
	@param parent_cluster 返回父目录的簇号
	@return 错误码
*/
static int32_t fat12_parse_path(FATFS *fs, const char *path, FAT_DIRENTRY *parent_dir, char *filename, uint16_t *parent_cluster)
{
	char temp_path[FAT_MAX_PATH];
	char *token;
	char *saveptr;
	uint16_t current_cluster = 0; /* 从根目录开始 */
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || path == NULL || filename == NULL || parent_cluster == NULL) {
		debug("FAT12: Invalid parameters in parse_path.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 复制路径，处理绝对路径和相对路径 */
	strncpy(temp_path, path, FAT_MAX_PATH - 1);
	temp_path[FAT_MAX_PATH - 1] = '\0';

	debug("FAT12: Parsing path: \'%s\'.\n", path);

	/* 路径以斜杠开头，从根目录开始 */
	if (temp_path[0] == '/')
		token = strtok_r(temp_path + 1, "/", &saveptr);
	else
		token = strtok_r(temp_path, "/", &saveptr);

	/* 没有路径组件 - 根目录 */
	if (!token) {
		*parent_cluster = 0;
		filename[0] = '\0';

		if (parent_dir != NULL)
			memset(parent_dir, 0, sizeof(FAT_DIRENTRY));

		debug("FAT12: Path refers to root directory.\n");
		return FATFS_SUCCESS;
	}

	/* 遍历路径组件，查找父目录 */
	for (;;) {
		char *next_token = strtok_r(NULL, "/", &saveptr);

		/* 最后一个组件 - 目标文件名 */
		if (!next_token) {
			strncpy(filename, token, FAT_MAX_PATH - 1);
			filename[FAT_MAX_PATH - 1] = '\0';
			*parent_cluster = current_cluster;

			/* 获取父目录的目录项 */
			if (parent_dir != NULL && current_cluster != 0) {
				/* 查找当前目录的目录项 */
				FAT_DIRENTRY dir_entry;
				ret = fat12_find_entry(fs, *parent_cluster, "..", &dir_entry, NULL);
				if (ret == FATFS_SUCCESS) {
					/* 获取父目录的实际目录项 */
					ret = fat12_find_entry(fs, dir_entry.first_cluster_low, ".", parent_dir, NULL);
					if (ret != FATFS_SUCCESS)
						memset(parent_dir, 0, sizeof(FAT_DIRENTRY));
				} else {
					memset(parent_dir, 0, sizeof(FAT_DIRENTRY));
				}
			} else if (parent_dir != NULL) {
				/* 根目录的父目录就是自身 */
				memset(parent_dir, 0, sizeof(FAT_DIRENTRY));
				parent_dir->attributes = FAT_ATTR_DIRECTORY;
			}

			debug("FAT12: Parsed path - parent cluster: %u, filename: \'%s\'.\n", *parent_cluster, filename);
			return FATFS_SUCCESS;
		}

		/* 否则，查找下一级目录 */
		FAT_DIRENTRY dir_entry;
		ret = fat12_find_entry(fs, current_cluster, token, &dir_entry, NULL);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Directory \'%s\' not found in cluster %u.\n", token, current_cluster);
			return FATFS_ENTRY_NOT_FOUND;
		}

		/* 检查是否为目录 */
		if (!(dir_entry.attributes & FAT_ATTR_DIRECTORY)) {
			debug("FAT12: \'%s\' is not a directory.\n", token);
			return FATFS_INVALID_PARAM;
		}

		/* 进入下一级目录 */
		current_cluster = dir_entry.first_cluster_low;
		token = next_token;
	}
}

/*
	@brief 更新文件时间戳。
	@param entry 目录项
	@param update_create 是否更新创建时间
	@param update_access 是否更新访问时间
	@param update_write 是否更新写入时间
*/
static void fat12_update_file_timestamp(FAT_DIRENTRY *entry, bool update_create, bool update_access, bool update_write)
{
	uint32_t year = get_year();
	uint32_t month = get_month();
	uint32_t day = get_day_of_month();
	uint32_t hour = get_hour();
	uint32_t minute = get_minute();
	uint32_t second = get_second();

	/* 转换为 FAT 时间格式 */
	uint16_t fat_time = ((hour & 0x1F) << 11) | ((minute & 0x3F) << 5) | ((second / 2) & 0x1F);
	uint16_t fat_date = (((year - 1980) & 0x7F) << 9) | ((month & 0x0F) << 5) | (day & 0x1F);

	if (update_create) {
		entry->creation_time_tenths = (second % 2) * 100; /* 10ms 精度 */
		entry->creation_time = fat_time;
		entry->creation_date = fat_date;
	}

	if (update_access) {
		entry->last_access_date = fat_date;
	}

	if (update_write) {
		entry->last_write_time = fat_time;
		entry->last_write_date = fat_date;
	}
}

/*
	@brief 更新目录项。
	@param fs 文件系统上下文
	@param parent_cluster 父目录簇号
	@param filename 文件名
	@param entry 新的目录项数据
	@return 错误码
*/
static int32_t fat12_update_directory_entry(FATFS *fs, uint16_t parent_cluster, const char *filename, FAT_DIRENTRY *entry)
{
	int32_t entry_index;
	int32_t ret;

	/* 查找目录项索引 */
	ret = fat12_find_entry(fs, parent_cluster, filename, NULL, &entry_index);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to find directory entry for update.\n");
		return ret;
	}

	/* 写入更新后的目录项 */
	ret = fat12_write_directory_entry(fs, parent_cluster, entry_index, entry);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to write updated directory entry.\n");
		return ret;
	}

	return FATFS_SUCCESS;
}

/*
	@brief 打开文件。
	@param file 文件句柄
	@param fs 文件系统上下文
	@param path 文件路径
	@return 错误码
*/
int32_t fat12_open_file(FAT_FILE *file, FATFS *fs, const char *path)
{
	char filename[FAT_MAX_PATH];
	uint16_t parent_cluster;
	int32_t entry_index;
	int32_t ret;

	/* 参数检查 */
	if (file == NULL || fs == NULL || path == NULL) {
		debug("FAT12: Invalid parameters in open.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 初始化文件句柄 */
	memset(file, 0, sizeof(FAT_FILE));
	file->fs = fs;
	strncpy(file->path, path, FAT_MAX_PATH - 1);
	file->path[FAT_MAX_PATH - 1] = '\0';

	/* 解析路径 */
	ret = fat12_parse_path(fs, path, NULL, filename, &parent_cluster);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to parse path: \'%s\'.\n", path);
		return ret;
	}

	/* 文件名为空（目录） */
	if (filename[0] == '\0') {
		debug("FAT12: Cannot open directory as file: \'%s\'.\n", path);
		return FATFS_INVALID_PARAM;
	}

	/* 查找文件 */
	file->entry = (FAT_DIRENTRY *) kmalloc(sizeof(FAT_DIRENTRY));
	if (file->entry == NULL) {
		debug("FAT12: Failed to allocate memory for directory entry.\n");
		return FATFS_MEMORY_ALLOC;
	}

	ret = fat12_find_entry(fs, parent_cluster, filename, file->entry, &entry_index);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: File not found: \'%s\'.\n", filename);
		kfree(file->entry);
		file->entry = NULL;
		return FATFS_ENTRY_NOT_FOUND;
	}

	/* 检查是否为文件 */
	if (file->entry->attributes & FAT_ATTR_DIRECTORY) {
		debug("FAT12: Path is a directory, not a file: \'%s\'.\n", path);
		kfree(file->entry);
		file->entry = NULL;
		return FATFS_IS_DIRECTORY;
	}

	/* 初始化文件句柄状态 */
	file->current_pos = 0;
	file->is_open = true;

	debug("FAT12: Opened file \'%s\' (size: %u bytes, start cluster: %u).\n",
		path, file->entry->file_size, file->entry->first_cluster_low);

	return FATFS_SUCCESS;
}

/*
	@brief 创建文件。
	@param fs 文件系统上下文
	@param path 文件路径
	@param file 返回的文件句柄
	@return 错误码
*/
int32_t fat12_create_file(FAT_FILE *file, FATFS *fs, const char *path)
{
	char filename[FAT_MAX_PATH];
	uint16_t parent_cluster;
	uint16_t start_cluster;
	FAT_DIRENTRY new_entry;
	int32_t ret;
	uint32_t bytes_written = 0; /* 记录写入的字节数 */

	/* 参数检查 */
	if (fs == NULL || path == NULL) {
		debug("FAT12: Invalid parameters in create_file.\n");
		return FATFS_INVALID_PARAM;
	}

	debug("FAT12: Creating file: \'%s\'.\n", path);

	/* 解析路径 */
	ret = fat12_parse_path(fs, path, NULL, filename, &parent_cluster);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to parse path: \'%s\'.\n", path);
		return ret;
	}

	/* 检查文件名是否为空 */
	if (filename[0] == '\0') {
		debug("FAT12: Cannot create file with empty name.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 检查文件是否已存在 */
	ret = fat12_find_entry(fs, parent_cluster, filename, NULL, NULL);
	if (ret == FATFS_SUCCESS) {
		debug("FAT12: File already exists: \'%s\'.\n", path);
		return FATFS_ENTRY_EXISTS;
	}

	/* 分配初始簇 */
	ret = fat12_allocate_cluster(fs, &start_cluster);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to allocate initial cluster for new file.\n");
		return ret;
	}

	/* 创建文件目录项 */
	ret = fat12_create_entry(fs, parent_cluster, filename, FAT_ATTR_ARCHIVE, start_cluster, 0, &new_entry);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to create directory entry for new file.\n");
		/* 回滚，释放已分配的簇 */
		fat12_free_clusters(fs, start_cluster);
		return ret;
	}

	/* 更新文件时间戳 */
	fat12_update_file_timestamp(&new_entry, true, true, true);

	/* 更新目录项 */
	ret = fat12_update_directory_entry(fs, parent_cluster, filename, &new_entry);
	if (ret != FATFS_SUCCESS)
		debug("FAT12: Failed to update directory entry timestamp.\n");

	if (file != NULL) {
		/* 初始化文件句柄 */
		memset(file, 0, sizeof(FAT_FILE));
		file->fs = fs;
		strncpy(file->path, path, FAT_MAX_PATH - 1);
		file->path[FAT_MAX_PATH - 1] = '\0';

		/* 分配并复制目录项 */
		file->entry = (FAT_DIRENTRY *) kmalloc(sizeof(FAT_DIRENTRY));
		if (file->entry == NULL) {
			debug("FAT12: Failed to allocate memory for file entry.\n");
		} else {
			memcpy(file->entry, &new_entry, sizeof(FAT_DIRENTRY));
			file->current_pos = 0;
			file->is_open = true;
		}
	}

	debug("FAT12: Created file \'%s\' with start cluster %u, bytes written: %u.\n", path, start_cluster, bytes_written);

	return FATFS_SUCCESS;
}

/*
	@brief 从文件读取数据。
	@param file 文件句柄
	@param buffer 数据缓冲区
	@param size 要读取的字节数
	@param bytes_read 返回实际读取的字节数
	@return 错误码
*/
int32_t fat12_read_file(FAT_FILE *file, void *buffer, uint32_t size, uint32_t *bytes_read)
{
	uint8_t *buf = (uint8_t *) buffer;
	uint32_t bytes_read_local = 0;
	uint32_t sector_size;
	uint32_t bytes_per_cluster;
	uint8_t *cluster_buffer = NULL;
	uint16_t current_cluster;
	uint32_t pos_in_file;
	uint32_t cluster_num;
	uint32_t offset_in_cluster;
	int32_t ret;

	/* 参数检查 */
	if (file == NULL || buffer == NULL || size == 0 || bytes_read == NULL) {
		debug("FAT12: Invalid parameters in read.\n");
		return FATFS_INVALID_PARAM;
	}

	*bytes_read = 0;

	if (!file->is_open) {
		debug("FAT12: File is not open.\n");
		return FATFS_INVALID_PARAM;
	}

	if (file->entry == NULL) {
		debug("FAT12: File has no directory entry.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 检查文件大小 */
	if (file->current_pos >= file->entry->file_size) {
		debug("FAT12: Already at end of file.\n");
		return FATFS_SUCCESS;
	}

	/* 计算大小 */
	sector_size = file->fs->bpb.bytes_per_sector;
	bytes_per_cluster = sector_size * file->fs->bpb.sectors_per_cluster;

	/* 分配簇缓冲区 */
	cluster_buffer = (uint8_t *) kmalloc(bytes_per_cluster);
	if (cluster_buffer == NULL) {
		debug("FAT12: Failed to allocate cluster buffer.\n");
		return FATFS_MEMORY_ALLOC;
	}

	/* 计算当前位置所在的簇和偏移 */
	pos_in_file = file->current_pos;
	cluster_num = pos_in_file / bytes_per_cluster;
	offset_in_cluster = pos_in_file % bytes_per_cluster;
	current_cluster = file->entry->first_cluster_low;

	debug("FAT12: Reading from file at position %u, cluster %u, offset %u.\n",
		pos_in_file, current_cluster, offset_in_cluster);

	/* 定位到当前簇 */
	for (uint32_t i = 0; i < cluster_num; i++) {
		ret = fat12_get_next_cluster(file->fs, current_cluster, &current_cluster);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to traverse cluster chain.\n");
			kfree(cluster_buffer);
			return ret;
		}

		if (current_cluster >= 0xFF8) {
			/* 已到达文件末尾 */
			debug("FAT12: Reached end of file while traversing cluster chain.\n");
			kfree(cluster_buffer);
			*bytes_read = bytes_read_local;
			return FATFS_SUCCESS;
		}
	}

	/* 读取数据 */
	while (bytes_read_local < size && pos_in_file < file->entry->file_size) {
		/* 读取当前簇 */
		ret = fat12_read_cluster(file->fs, current_cluster, cluster_buffer);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to read cluster %u.\n", current_cluster);
			break;
		}

		/* 计算本次读取的字节数 */
		uint32_t bytes_to_read = size - bytes_read_local;
		uint32_t remaining_in_cluster = bytes_per_cluster - offset_in_cluster;
		uint32_t remaining_in_file = file->entry->file_size - pos_in_file;

		if (bytes_to_read > remaining_in_cluster)
			bytes_to_read = remaining_in_cluster;
		if (bytes_to_read > remaining_in_file)
			bytes_to_read = remaining_in_file;

		/* 复制数据 */
		memcpy(buf + bytes_read_local, cluster_buffer + offset_in_cluster, bytes_to_read);

		/* 更新计数和位置 */
		bytes_read_local += bytes_to_read;
		pos_in_file += bytes_to_read;
		file->current_pos = pos_in_file;
		offset_in_cluster += bytes_to_read;

		/* 已读完当前簇，移动到下一个簇 */
		if (offset_in_cluster >= bytes_per_cluster) {
			ret = fat12_get_next_cluster(file->fs, current_cluster, &current_cluster);
			if (ret != FATFS_SUCCESS || current_cluster >= 0xFF8) {
				/* 到达文件末尾或出错 */
				break;
			}
			offset_in_cluster = 0;
		}
	}

	/* 释放簇缓冲区 */
	kfree(cluster_buffer);

	*bytes_read = bytes_read_local;

	/* 更新文件访问时间 */
	if (bytes_read_local > 0) {
		fat12_update_file_timestamp(file->entry, false, true, false);

		/* 更新目录项 */
		char filename[FAT_MAX_PATH];
		uint16_t parent_cluster;
		ret = fat12_parse_path(file->fs, file->path, NULL, filename, &parent_cluster);
		if (ret == FATFS_SUCCESS && filename[0] != '\0')
			fat12_update_directory_entry(file->fs, parent_cluster, filename, file->entry);
	}

	debug("FAT12: Read %u bytes from file, new position: %u.\n", bytes_read_local, file->current_pos);

	return FATFS_SUCCESS;
}

/*
	@brief 向文件写入数据。
	@param file 文件句柄
	@param buffer 数据缓冲区
	@param size 要写入的字节数
	@param append 是否为追加模式
	@param bytes_written 返回实际写入的字节数
	@return 错误码
*/
int32_t fat12_write_file(FAT_FILE *file, const void *buffer, uint32_t size, bool append, uint32_t *bytes_written)
{
	const uint8_t *buf = (const uint8_t *) buffer;
	uint32_t bytes_written_local = 0;
	uint32_t sector_size;
	uint32_t bytes_per_cluster;
	uint8_t *cluster_buffer = NULL;
	uint16_t current_cluster;
	uint32_t pos_in_file;
	uint32_t cluster_num;
	uint32_t offset_in_cluster;
	int32_t ret;

	/* 参数检查 */
	if (file == NULL || buffer == NULL || size == 0 || bytes_written == NULL) {
		debug("FAT12: Invalid parameters in write.\n");
		return FATFS_INVALID_PARAM;
	}

	*bytes_written = 0;

	if (!file->is_open) {
		debug("FAT12: File is not open.\n");
		return FATFS_INVALID_PARAM;
	}

	if (file->entry == NULL) {
		debug("FAT12: File has no directory entry.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 计算大小 */
	sector_size = file->fs->bpb.bytes_per_sector;
	bytes_per_cluster = sector_size * file->fs->bpb.sectors_per_cluster;

	/* 分配簇缓冲区 */
	cluster_buffer = (uint8_t *) kmalloc(bytes_per_cluster);
	if (cluster_buffer == NULL) {
		debug("FAT12: Failed to allocate cluster buffer.\n");
		return FATFS_MEMORY_ALLOC;
	}

	/* 覆盖模式 */
	if (!append) {
		file->current_pos = 0;

		/* 文件已有数据，释放除第一个簇外的所有簇 */
		if (file->entry->first_cluster_low != 0) {
			uint16_t next_cluster;
			ret = fat12_get_next_cluster(file->fs, file->entry->first_cluster_low, &next_cluster);
			if (ret == FATFS_SUCCESS && next_cluster < 0xFF8) {
				ret = fat12_free_clusters(file->fs, next_cluster);
				if (ret != FATFS_SUCCESS) {
					debug("FAT12: Warning - failed to free existing clusters.\n");
				}
			}

			/* 标记第一个簇为结束 */
			ret = fat12_set_next_cluster(file->fs, file->entry->first_cluster_low, 0xFFF);
			if (ret != FATFS_SUCCESS) {
				debug("FAT12: Failed to mark first cluster as end.\n");
				kfree(cluster_buffer);
				return ret;
			}
		}

		file->entry->file_size = 0;
	}

	/* 计算当前位置 */
	pos_in_file = file->current_pos;
	cluster_num = pos_in_file / bytes_per_cluster;
	offset_in_cluster = pos_in_file % bytes_per_cluster;
	current_cluster = file->entry->first_cluster_low;

	/* 文件没有簇，分配第一个簇 */
	if (current_cluster == 0) {
		ret = fat12_allocate_cluster(file->fs, &current_cluster);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to allocate first cluster for file.\n");
			kfree(cluster_buffer);
			return ret;
		}
		file->entry->first_cluster_low = current_cluster;

		/* 标记为簇链结束 */
		ret = fat12_set_next_cluster(file->fs, current_cluster, 0xFFF);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to mark first cluster as end.\n");
			kfree(cluster_buffer);
			return ret;
		}
	}

	debug("FAT12: Writing to file at position %u, cluster %u, offset %u.\n",
		pos_in_file, current_cluster, offset_in_cluster);

	/* 定位到当前簇 */
	for (uint32_t i = 0; i < cluster_num; i++) {
		uint16_t next_cluster;
		ret = fat12_get_next_cluster(file->fs, current_cluster, &next_cluster);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to traverse cluster chain.\n");
			kfree(cluster_buffer);
			return ret;
		}

		if (next_cluster >= 0xFF8) {
			/* 需要分配新簇 */
			ret = fat12_allocate_cluster(file->fs, &next_cluster);
			if (ret != FATFS_SUCCESS) {
				debug("FAT12: Failed to allocate new cluster.\n");
				kfree(cluster_buffer);
				return ret;
			}

			ret = fat12_set_next_cluster(file->fs, current_cluster, next_cluster);
			if (ret != FATFS_SUCCESS) {
				debug("FAT12: Failed to link new cluster.\n");
				kfree(cluster_buffer);
				return ret;
			}

			/* 标记新簇为结束 */
			ret = fat12_set_next_cluster(file->fs, next_cluster, 0xFFF);
			if (ret != FATFS_SUCCESS) {
				debug("FAT12: Failed to mark new cluster as end.\n");
				kfree(cluster_buffer);
				return ret;
			}
		}
		current_cluster = next_cluster;
	}

	/* 写入数据 */
	while (bytes_written_local < size) {
		/* 需要修改簇的中间部分，先读取整个簇 */
		if (offset_in_cluster > 0 || (offset_in_cluster + (size - bytes_written_local)) < bytes_per_cluster) {
			ret = fat12_read_cluster(file->fs, current_cluster, cluster_buffer);
			if (ret != FATFS_SUCCESS) {
				debug("FAT12: Failed to read cluster for partial write.\n");
				break;
			}
		} else {
			/* 对于新簇或完整簇写入，清零簇缓冲区 */
			memset(cluster_buffer, 0, bytes_per_cluster);
		}

		/* 计算本次写入的字节数 */
		uint32_t bytes_to_write = size - bytes_written_local;
		uint32_t remaining_in_cluster = bytes_per_cluster - offset_in_cluster;

		if (bytes_to_write > remaining_in_cluster)
			bytes_to_write = remaining_in_cluster;

		/* 复制数据到簇缓冲区 */
		memcpy(cluster_buffer + offset_in_cluster, buf + bytes_written_local, bytes_to_write);

		/* 写入簇 */
		ret = fat12_write_cluster(file->fs, current_cluster, cluster_buffer);
		if (ret != FATFS_SUCCESS) {
			debug("FAT12: Failed to write cluster %u.\n", current_cluster);
			break;
		}

		/* 更新计数和位置 */
		bytes_written_local += bytes_to_write;
		pos_in_file += bytes_to_write;
		file->current_pos = pos_in_file;
		offset_in_cluster += bytes_to_write;

		/* 更新文件大小 */
		if (pos_in_file > file->entry->file_size)
			file->entry->file_size = pos_in_file;

		/* 已写满当前簇，移动到下一个簇 */
		if (offset_in_cluster >= bytes_per_cluster) {
			uint16_t next_cluster;
			ret = fat12_get_next_cluster(file->fs, current_cluster, &next_cluster);
			if (ret != FATFS_SUCCESS || next_cluster >= 0xFF8) {
				/* 需要分配新簇 */
				ret = fat12_allocate_cluster(file->fs, &next_cluster);
				if (ret != FATFS_SUCCESS) {
					debug("FAT12: Failed to allocate new cluster.\n");
					break;
				}

				ret = fat12_set_next_cluster(file->fs, current_cluster, next_cluster);
				if (ret != FATFS_SUCCESS) {
					debug("FAT12: Failed to link new cluster.\n");
					break;
				}

				/* 标记新簇为结束 */
				ret = fat12_set_next_cluster(file->fs, next_cluster, 0xFFF);
				if (ret != FATFS_SUCCESS) {
					debug("FAT12: Failed to mark new cluster as end.\n");
					break;
				}
			}
			current_cluster = next_cluster;
			offset_in_cluster = 0;
		}
	}

	/* 释放簇缓冲区 */
	kfree(cluster_buffer);

	*bytes_written = bytes_written_local;

	/* 更新文件时间戳 */
	if (bytes_written_local > 0) {
		fat12_update_file_timestamp(file->entry, false, false, true);

		/* 更新目录项 */
		char filename[FAT_MAX_PATH];
		uint16_t parent_cluster;
		ret = fat12_parse_path(file->fs, file->path, NULL, filename, &parent_cluster);
		if (ret == FATFS_SUCCESS && filename[0] != '\0') {
			fat12_update_directory_entry(file->fs, parent_cluster, filename, file->entry);
		}
	}

	debug("FAT12: Wrote %u bytes to file, new size: %u, new position: %u.\n",
		bytes_written_local, file->entry->file_size, file->current_pos);

	return FATFS_SUCCESS;
}

/*
	@brief 关闭文件。
	@param file 文件句柄
	@return 错误码
*/
int32_t fat12_close_file(FAT_FILE *file)
{
	/* 参数检查 */
	if (file == NULL) {
		debug("FAT12: Invalid parameters in close.\n");
		return FATFS_INVALID_PARAM;
	}

	if (!file->is_open) {
		debug("FAT12: File is not open.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 更新文件访问时间 */
	if (file->current_pos > 0) {
		fat12_update_file_timestamp(file->entry, false, true, true);

		/* 更新目录项 */
		char filename[FAT_MAX_PATH];
		uint16_t parent_cluster;
		int32_t ret = fat12_parse_path(file->fs, file->path, NULL, filename, &parent_cluster);
		if (ret == FATFS_SUCCESS && filename[0] != '\0') {
			fat12_update_directory_entry(file->fs, parent_cluster, filename, file->entry);
		}
	}

	/* 释放目录项内存 */
	if (file->entry != NULL) {
		kfree(file->entry);
		file->entry = NULL;
	}

	/* 关闭文件句柄 */
	file->is_open = false;
	file->fs = NULL;
	file->current_pos = 0;
	file->path[0] = '\0';

	debug("FAT12: Closed file.\n");

	return FATFS_SUCCESS;
}

/*
	@brief 删除文件。
	@param fs 文件系统上下文
	@param path 文件路径
	@return 错误码
*/
int32_t fat12_delete_file(FATFS *fs, const char *path)
{
	char filename[FAT_MAX_PATH];
	uint16_t parent_cluster;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || path == NULL) {
		debug("FAT12: Invalid parameters in delete_file.\n");
		return FATFS_INVALID_PARAM;
	}

	debug("FAT12: Deleting file: \'%s\'.\n", path);

	/* 解析路径 */
	ret = fat12_parse_path(fs, path, NULL, filename, &parent_cluster);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to parse path: \'%s\'.\n", path);
		return ret;
	}

	/* 检查文件名是否为空（目录） */
	if (filename[0] == '\0') {
		debug("FAT12: Cannot delete directory.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 删除文件目录项 */
	ret = fat12_delete_entry(fs, parent_cluster, filename);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to delete file entry.\n");
		return ret;
	}

	debug("FAT12: Successfully deleted file: \'%s\'.\n", path);

	return FATFS_SUCCESS;
}

/*
	@brief 创建目录。
	@param fs 文件系统上下文
	@param path 目录路径
	@return 错误码
*/
int32_t fat12_create_directory(FATFS *fs, const char *path)
{
	char dirname[FAT_MAX_PATH];
	uint16_t parent_cluster;
	uint16_t start_cluster;
	FAT_DIRENTRY new_entry;
	uint8_t *cluster_buffer = NULL;
	uint32_t bytes_per_cluster;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || path == NULL) {
		debug("FAT12: Invalid parameters in create_directory.\n");
		return FATFS_INVALID_PARAM;
	}

	debug("FAT12: Creating directory: \'%s\'.\n", path);

	/* 解析路径 */
	ret = fat12_parse_path(fs, path, NULL, dirname, &parent_cluster);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to parse path: \'%s\'.\n", path);
		return ret;
	}

	/* 检查目录名是否为空 */
	if (dirname[0] == '\0') {
		debug("FAT12: Cannot create directory with empty name.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 检查目录是否已存在 */
	ret = fat12_find_entry(fs, parent_cluster, dirname, NULL, NULL);
	if (ret == FATFS_SUCCESS) {
		debug("FAT12: Directory already exists: \'%s\'.\n", path);
		return FATFS_ENTRY_EXISTS;
	}

	/* 分配簇 */
	ret = fat12_allocate_cluster(fs, &start_cluster);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to allocate cluster for new directory.\n");
		return ret;
	}

	/* 创建目录项 */
	ret = fat12_create_entry(fs, parent_cluster, dirname, FAT_ATTR_DIRECTORY, start_cluster, 0, &new_entry);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to create directory entry.\n");
		fat12_free_clusters(fs, start_cluster);
		return ret;
	}

	/* 更新目录时间戳 */
	fat12_update_file_timestamp(&new_entry, true, true, true);

	/* 更新目录项 */
	ret = fat12_update_directory_entry(fs, parent_cluster, dirname, &new_entry);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to update directory entry timestamp.\n");
	}

	/* 分配簇缓冲区 */
	bytes_per_cluster = fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster;
	cluster_buffer = (uint8_t *) kmalloc(bytes_per_cluster);
	if (cluster_buffer == NULL) {
		debug("FAT12: Failed to allocate cluster buffer.\n");
		fat12_delete_entry(fs, parent_cluster, dirname);
		fat12_free_clusters(fs, start_cluster);
		return FATFS_MEMORY_ALLOC;
	}

	/* 初始化簇缓冲区为零 */
	memset(cluster_buffer, 0, bytes_per_cluster);

	/* 创建 . 目录项（指向自身） */
	FAT_DIRENTRY *dot_entry = (FAT_DIRENTRY *) cluster_buffer;
	memset(dot_entry, 0, sizeof(FAT_DIRENTRY));
	memcpy(dot_entry->filename, ".       ", FAT_FILENAME_LEN);
	memcpy(dot_entry->extension, "   ", FAT_EXT_LEN);
	dot_entry->attributes = FAT_ATTR_DIRECTORY;
	dot_entry->first_cluster_low = start_cluster;
	fat12_update_file_timestamp(dot_entry, true, true, true);

	/* 创建 .. 目录项（指向父目录） */
	FAT_DIRENTRY *dotdot_entry = (FAT_DIRENTRY *) (cluster_buffer + sizeof(FAT_DIRENTRY));
	memset(dotdot_entry, 0, sizeof(FAT_DIRENTRY));
	memcpy(dotdot_entry->filename, "..      ", FAT_FILENAME_LEN);
	memcpy(dotdot_entry->extension, "   ", FAT_EXT_LEN);
	dotdot_entry->attributes = FAT_ATTR_DIRECTORY;
	dotdot_entry->first_cluster_low = parent_cluster;
	fat12_update_file_timestamp(dotdot_entry, true, true, true);

	/* 写入目录簇 */
	ret = fat12_write_cluster(fs, start_cluster, cluster_buffer);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to write directory cluster.\n");
		kfree(cluster_buffer);
		fat12_delete_entry(fs, parent_cluster, dirname);
		fat12_free_clusters(fs, start_cluster);
		return ret;
	}

	/* 释放簇缓冲区 */
	kfree(cluster_buffer);

	debug("FAT12: Created directory \'%s\' with start cluster %u.\n", path, start_cluster);

	return FATFS_SUCCESS;
}

/*
	@brief 删除目录。
	@param fs 文件系统上下文
	@param path 目录路径
	@return 错误码
*/
int32_t fat12_delete_directory(FATFS *fs, const char *path)
{
	char dirname[FAT_MAX_PATH];
	uint16_t parent_cluster;
	FAT_DIRENTRY dir_entry;
	FAT_DIRENTRY entries[16];
	int32_t count;
	int32_t ret;

	/* 参数检查 */
	if (fs == NULL || path == NULL) {
		debug("FAT12: Invalid parameters in delete_directory.\n");
		return FATFS_INVALID_PARAM;
	}

	debug("FAT12: Deleting directory: \'%s\'.\n", path);

	/* 解析路径 */
	ret = fat12_parse_path(fs, path, NULL, dirname, &parent_cluster);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to parse path: \'%s\'.\n", path);
		return ret;
	}

	/* 检查目录名是否为空 */
	if (dirname[0] == '\0') {
		debug("FAT12: Cannot delete root directory.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 查找目录 */
	ret = fat12_find_entry(fs, parent_cluster, dirname, &dir_entry, NULL);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Directory not found: \'%s\'.\n", path);
		return FATFS_ENTRY_NOT_FOUND;
	}

	/* 检查是否为目录 */
	if (!(dir_entry.attributes & FAT_ATTR_DIRECTORY)) {
		debug("FAT12: Path is not a directory: \'%s\'.\n", path);
		return FATFS_INVALID_PARAM;
	}

	/* 检查目录是否为空 */
	fat12_read_directory(fs, dir_entry.first_cluster_low, entries, 16, &count);
	if (count < 0) {
		debug("FAT12: Failed to read directory contents.\n");
		return FATFS_READ_FAILED;
	}

	/* 目录应至少包含 . 和 .. 两个条目 */
	if (count > 2) {
		debug("FAT12: Directory is not empty: \'%s\' (%d entries).\n", path, count);
		return FATFS_DIR_NOT_EMPTY;
	}

	/* 删除目录项 */
	ret = fat12_delete_entry(fs, parent_cluster, dirname);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to delete directory entry.\n");
		return ret;
	}

	debug("FAT12: Successfully deleted directory: \'%s\'.\n", path);

	return FATFS_SUCCESS;
}

/*
	@brief 获取目录条目。
	@param fs 文件系统上下文
	@param path 目录路径
	@param entries 目录项数组
	@param max_entries 最大目录项数
	@param entries_read 返回实际读取的目录项数
	@return 错误码
*/
int32_t fat12_get_directory_entries(FATFS *fs, const char *path, FAT_DIRENTRY *entries, int32_t max_entries, int32_t *entries_read)
{
	char dirname[FAT_MAX_PATH];
	uint16_t parent_cluster;
	FAT_DIRENTRY dir_entry;
	int32_t ret;
	int32_t local_entries_read = 0;

	/* 参数检查 */
	if (fs == NULL || path == NULL || entries == NULL || max_entries <= 0 || entries_read == NULL) {
		debug("FAT12: Invalid parameters in get_directory_entries.\n");
		return FATFS_INVALID_PARAM;
	}

	*entries_read = 0;

	debug("FAT12: Getting directory entries for: \'%s\'.\n", path);

	/* 解析路径 */
	ret = fat12_parse_path(fs, path, NULL, dirname, &parent_cluster);
	if (ret != FATFS_SUCCESS) {
		/* 解析失败，尝试将路径作为根目录处理 */
		if (strlen(path) == 0 || (path[0] == '/' && strlen(path) == 1)) {
			/* 根目录 */
			debug("FAT12: Reading root directory.\n");
			ret = fat12_read_directory(fs, 0, entries, max_entries, &local_entries_read);
			*entries_read = local_entries_read;
			return ret;
		}

		/* 尝试在当前目录查找 */
		strncpy(dirname, path, FAT_MAX_PATH - 1);
		dirname[FAT_MAX_PATH - 1] = '\0';
		parent_cluster = 0; /* 默认从根目录查找 */
	} else {
		/* 解析成功但文件名为空（目录） */
		if (dirname[0] == '\0') {
			debug("FAT12: Reading root directory.\n");
			ret = fat12_read_directory(fs, 0, entries, max_entries, &local_entries_read);
			*entries_read = local_entries_read;
			return ret;
		}
	}

	/* 查找目录 */
	ret = fat12_find_entry(fs, parent_cluster, dirname, &dir_entry, NULL);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Directory not found: \'%s\'.\n", path);
		return FATFS_ENTRY_NOT_FOUND;
	}

	/* 检查是否为目录 */
	if (!(dir_entry.attributes & FAT_ATTR_DIRECTORY)) {
		debug("FAT12: Path is not a directory: \'%s\'.\n", path);
		return FATFS_INVALID_PARAM;
	}

	/* 更新目录访问时间 */
	fat12_update_file_timestamp(&dir_entry, false, true, false);

	/* 更新目录项 */
	ret = fat12_update_directory_entry(fs, parent_cluster, dirname, &dir_entry);
	if (ret != FATFS_SUCCESS)
		debug("FAT12: Failed to update directory access time.\n");

	/* 读取目录内容 */
	ret = fat12_read_directory(fs, dir_entry.first_cluster_low, entries, max_entries, &local_entries_read);
	if (ret != FATFS_SUCCESS) {
		debug("FAT12: Failed to read directory contents.\n");
		return ret;
	}

	*entries_read = local_entries_read;

	debug("FAT12: Read %d entries from directory \'%s\'.\n", local_entries_read, path);

	return FATFS_SUCCESS;
}
