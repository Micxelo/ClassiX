/*
	utilities/fs/fatfs.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/fatfs.h>
#include <ClassiX/typedef.h>

#include <ctype.h>
#include <string.h>

void test_mbr(BLKDEV *device)
{
	MBR mbr;
	device->read(device, 0, 1, (uint8_t *) &mbr);
	if (mbr.signature != MBR_SIGNATURE) {
		debug("Invalid MBR signature %04x\n", mbr.signature);
		return;
	}

	debug("MBR signature valid: %04x\n", mbr.signature);
	
	for (int32_t i = 0; i < 4; i++) {
		PART_ENTRY *p = &mbr.partitions[i];

		if (p->type == PART_TYPE_EMPTY)
			continue;

		debug("Partition %d:\n", i);
		debug("  Boot flag: %02x\n", p->bootable);
		debug("  Type: %02x\n", p->type);
		debug("  Start LBA: %u\n", p->start_lba);
		debug("  Sector count: %u\n", p->sector_count);
		
		// 检查是否为FAT12/FAT16分区
		if (p->type == PART_TYPE_FAT12 || p->type == PART_TYPE_FAT16 || 
			p->type == PART_TYPE_FAT16B || p->type == PART_TYPE_FAT16_LBA) {
			
			debug("  *** FAT12/FAT16 filesystem found ***\n");
			
			// 读取分区引导扇区
			FAT_BPB boot;
			if (device->read(device, p->start_lba, 1, (uint8_t *)&boot) != BD_SUCCESS) {
				debug("  Failed to read boot sector\n");
				continue;
			}
			
			// 验证引导扇区签名
			if (boot.signature != MBR_SIGNATURE) {
				debug("  Invalid boot sector signature: %04x\n", boot.signature);
				continue;
			}
			
			debug("  Boot sector information:\n");
			debug("    Bytes per sector: %u\n", boot.bytes_per_sector);
			debug("    Sectors per cluster: %u\n", boot.sectors_per_cluster);
			debug("    Reserved sectors: %u\n", boot.reserved_sectors);
			debug("    Number of FATs: %u\n", boot.fat_count);
			debug("    Root entries: %u\n", boot.root_entries);
			debug("    Total sectors (16): %u\n", boot.total_sectors_short);
			debug("    Total sectors (32): %u\n", boot.total_sectors_long);
			debug("    FAT size (sectors): %u\n", boot.sectors_per_fat);
			
			// 计算文件系统参数
			uint32_t total_sectors = boot.total_sectors_short;
			if (total_sectors == 0) {
				total_sectors = boot.total_sectors_long;
			}
			
			uint32_t root_dir_sectors = ((boot.root_entries * 32) + (boot.bytes_per_sector - 1)) / boot.bytes_per_sector;
			uint32_t data_sectors = total_sectors - (boot.reserved_sectors + boot.fat_count * boot.sectors_per_fat + root_dir_sectors);
			uint32_t total_clusters = data_sectors / boot.sectors_per_cluster;
			
			// 确定FAT类型
			const char* fat_type;
			if (total_clusters < 4085) {
				fat_type = "FAT12";
			} else if (total_clusters < 65525) {
				fat_type = "FAT16";
			} else {
				fat_type = "FAT32?";
			}
			
			debug("    Total clusters: %u\n", total_clusters);
			debug("    Detected type: %s\n", fat_type);
			debug("    Volume label: %.11s\n", boot.volume_label);
			debug("    File system type: %.8s\n", boot.fs_type);
			
			// 计算关键位置
			uint32_t fat1_start = p->start_lba + boot.reserved_sectors;
			uint32_t fat2_start = fat1_start + boot.sectors_per_fat;
			uint32_t root_dir_start = fat2_start + boot.sectors_per_fat;
			uint32_t data_start = root_dir_start + root_dir_sectors;
			
			debug("    FAT1 start sector: %u\n", fat1_start);
			debug("    FAT2 start sector: %u\n", fat2_start);
			debug("    Root directory start sector: %u\n", root_dir_start);
			debug("    Data area start sector: %u\n", data_start);
			
			// 如果是第一个找到的FAT分区，可以在这里设置全局变量或返回信息
			// 例如: found_fat_partition = p->start_lba;
		}
		else {
			debug("  Not a FAT12/FAT16 partition\n");
		}
		
		debug("\n");
	}
}

/*
	@brief 将长文件名转换为 8.3 格式。
	@param filename 输入文件名
	@param short_name 输出的 8 字符主文件名
	@param extension 输出的 3 字符扩展名
*/
void fat12_convert_to_83(const char *filename, char *short_name, char *extension)
{
	/* 参数检查 */
	if (filename == NULL || short_name == NULL || extension == NULL) 
		return;
	
	/* 初始化短文件名和扩展名 */
	memset(short_name, ' ', FAT_FILENAME_LEN);
	memset(extension, ' ', FAT_EXT_LEN);
	short_name[FAT_FILENAME_LEN] = '\0';
	extension[FAT_EXT_LEN] = '\0';
	
	/* 处理以点开头的文件 */
	if (filename[0] == '.') {
		const char *ext_start = filename + 1;
		int32_t ext_len = strlen(ext_start);
		int32_t copy_len = (ext_len < FAT_EXT_LEN) ? ext_len : FAT_EXT_LEN;
		
		for (int32_t i = 0; i < copy_len; i++)
			extension[i] = toupper(ext_start[i]);
		
		return;
	}
	
	/* 从右边查找点 */
	const char *last_dot = strrchr(filename, '.');
	
	if (last_dot != NULL) {
		/* 有扩展名 */
		int32_t name_len = last_dot - filename;
		int32_t ext_len = strlen(last_dot + 1);
		
		/* 复制主文件名部分 */
		int32_t copy_len = (name_len < FAT_FILENAME_LEN) ? name_len : FAT_FILENAME_LEN;
		for (int32_t i = 0; i < copy_len; i++)
			short_name[i] = toupper(filename[i]);
		
		/* 复制扩展名部分 */
		copy_len = (ext_len < FAT_EXT_LEN) ? ext_len : FAT_EXT_LEN;
		for (int32_t i = 0; i < copy_len; i++)
			extension[i] = toupper((last_dot + 1)[i]);
	} else {
		/* 无扩展名 */
		int32_t name_len = strlen(filename);
		int32_t copy_len = (name_len < FAT_FILENAME_LEN) ? name_len : FAT_FILENAME_LEN;
		
		for (int32_t i = 0; i < copy_len; i++)
			short_name[i] = toupper(filename[i]);
	}
}

/*
	@brief 将 8.3 格式转换为长文件名。
	@param short_name 8 字符主文件名
	@param extension 3 字符扩展名
	@param filename 输出文件名缓冲区
*/
void fat12_convert_from_83(const char *short_name, const char *extension, char *filename)
{
	/* 参数检查 */
	if (short_name == NULL || extension == NULL || filename == NULL)
		return;
	
	int32_t j = 0;
	
	/* 检查是否仅有扩展名 */
	bool name_is_blank = true;
	for (int32_t i = 0; i < FAT_FILENAME_LEN; i++)
		if (short_name[i] != ' ') {
			name_is_blank = false;
			break;
		}
	
	if (name_is_blank) {
		filename[j++] = '.';
		
		/* 复制扩展名部分，去除尾部空格 */
		for (int32_t i = 0; i < FAT_EXT_LEN; i++)
			if (extension[i] != ' ')
				filename[j++] = tolower(extension[i]);
		
		filename[j] = '\0';
		return;
	}
	
	/* 复制文件名部分，去除尾部空格 */
	for (int32_t i = 0; i < FAT_FILENAME_LEN; i++)
		if (short_name[i] != ' ')
			filename[j++] = tolower(short_name[i]);
	
	/* 检查是否有扩展名 */
	bool has_extension = false;
	for (int32_t i = 0; i < FAT_EXT_LEN; i++)
		if (extension[i] != ' ') {
			has_extension = true;
			break;
		}
	
	/* 添加点和扩展名 */
	if (has_extension) {
		filename[j++] = '.';
		for (int32_t i = 0; i < FAT_EXT_LEN; i++)
			if (extension[i] != ' ')
				filename[j++] = tolower(extension[i]);
	}
	
	filename[j] = '\0';
}
