/*
	utilities/fs/fat12.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/fatfs.h>
#include <ClassiX/memory.h>
#include <ClassiX/typedef.h>

#include <string.h>

#define ROOT_DIR_ENTRY_COUNT				224		/* 根目录项数 */
#define FAT_ENTRY_SIZE						3		/* 单个 FAT 项占用 1.5 字节 */

/* 从设备读取一个扇区 */
static inline int32_t read_sector(const FATFS *fs, uint32_t sector, uint8_t *buffer)
{
	return fs->device->read(fs->device, sector + fs->fs_sector, 1, buffer);
}

/* 向设备写入一个扇区 */
static inline int32_t write_sector(const FATFS *fs, uint32_t sector, const uint8_t *buffer)
{
	return fs->device->write(fs->device, sector + fs->fs_sector, 1, buffer);
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
	root_dir_sectors = ((bpb->root_entries * sizeof(DIRENTRY)) + bpb->bytes_per_sector - 1) / bpb->bytes_per_sector;

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
		if (strncmp(fat12_fs_types[i], (const char *)bpb->fs_type, strlen(fat12_fs_types[i])) == 0) {
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

		/* 如果不是有效的 BPB，则查找 MBR 中的 FAT12 分区 */
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
	root_dir_sectors = ((fs->bpb.root_entries * sizeof(DIRENTRY)) + fs->bpb.bytes_per_sector - 1) / fs->bpb.bytes_per_sector;

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
	if (read_sector(fs, fs->bpb.reserved_sectors, fs->fat) != BD_SUCCESS) {
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

			debug("FAT12: Writing back FAT table %u at sector %u。\n", i, current_fat_sector);

			/* 写入当前 FAT 表 */
			if (write_sector(fs, current_fat_sector, fs->fat) != BD_SUCCESS) {
				debug("FAT12: Failed to write FAT table %u。\n", i);
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
static int32_t fat12_get_next_cluster(FATFS *fs, uint16_t cluster, uint16_t *next_cluster)
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
		ret = read_sector(fs, sector + i, buffer + i * bytes_per_sector);
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
		ret = write_sector(fs, sector + i, buffer + i * bytes_per_sector);
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
static int32_t fat12_read_directory_entry(FATFS *fs, uint16_t dir_cluster, int32_t entry_num, DIRENTRY *entry)
{
	uint8_t buffer[512];
	int32_t entries_per_sector = fs->bpb.bytes_per_sector / sizeof(DIRENTRY);
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
		int32_t entries_per_cluster = (fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster) / sizeof(DIRENTRY);
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
	ret = read_sector(fs, sector_num, buffer);
	if (ret != BD_SUCCESS) {
		debug("FAT12: Failed to read sector %d for directory entry.\n", sector_num);
		return FATFS_READ_FAILED;
	}
	
	memcpy(entry, &buffer[sector_offset * sizeof(DIRENTRY)], sizeof(DIRENTRY));
	
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
static int32_t fat12_write_directory_entry(FATFS *fs, uint16_t dir_cluster, int32_t entry_num, const DIRENTRY *entry)
{
	uint8_t buffer[512];
	int32_t entries_per_sector = fs->bpb.bytes_per_sector / sizeof(DIRENTRY);
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
		int32_t entries_per_cluster = (fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster) / sizeof(DIRENTRY);
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
	ret = read_sector(fs, sector_num, buffer);
	if (ret != BD_SUCCESS) {
		debug("FAT12: Failed to read sector %d for directory entry update.\n", sector_num);
		return FATFS_READ_FAILED;
	}
	
	memcpy(&buffer[sector_offset * sizeof(DIRENTRY)], entry, sizeof(DIRENTRY));
	
	ret = write_sector(fs, sector_num, buffer);
	if (ret != BD_SUCCESS) {
		debug("FAT12: Failed to write sector %d with updated directory entry.\n", sector_num);
		return FATFS_WRITE_FAILED;
	}
	
	debug("FAT12: Wrote directory entry %d to sector %d, offset %d.\n", entry_num, sector_num, sector_offset);
	
	return FATFS_SUCCESS;
}

