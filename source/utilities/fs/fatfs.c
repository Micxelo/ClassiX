/*
	utilities/fs/fatfs.c
*/

#include <ClassiX/blkdev.h>
#include <ClassiX/debug.h>
#include <ClassiX/fatfs.h>
#include <ClassiX/typedef.h>

#include <ctype.h>
#include <string.h>

static FATFS rootfs;
FATFS * const g_fs = &rootfs;

int32_t fat12_get_next_cluster(FATFS *fs, uint16_t cluster, uint16_t *next_cluster);
int32_t fat12_init(FATFS *fs, BLKDEV *device);
int32_t fat12_close(FATFS *fs);
int32_t fat12_open_file(FAT_FILE *file, FATFS *fs, const char *path);
int32_t fat12_create_file(FAT_FILE *file, FATFS *fs, const char *path);
int32_t fat12_read_file(FAT_FILE *file, void *buffer, uint32_t size, uint32_t *bytes_read);
int32_t fat12_write_file(FAT_FILE *file, const void *buffer, uint32_t size, bool append, uint32_t *bytes_written);
int32_t fat12_close_file(FAT_FILE *file);
int32_t fat12_delete_file(FATFS *fs, const char *path);
int32_t fat12_create_directory(FATFS *fs, const char *path);
int32_t fat12_delete_directory(FATFS *fs, const char *path);
int32_t fat12_get_directory_entries(FATFS *fs, const char *path, FAT_DIRENTRY *entries, int32_t max_entries, int32_t *entries_read);

int32_t fat16_get_next_cluster(FATFS *fs, uint16_t cluster, uint16_t *next_cluster);
int32_t fat16_init(FATFS *fs, BLKDEV *device);
int32_t fat16_close(FATFS *fs);
int32_t fat16_open_file(FAT_FILE *file, FATFS *fs, const char *path);
int32_t fat16_create_file(FAT_FILE *file, FATFS *fs, const char *path);
int32_t fat16_read_file(FAT_FILE *file, void *buffer, uint32_t size, uint32_t *bytes_read);
int32_t fat16_write_file(FAT_FILE *file, const void *buffer, uint32_t size, bool append, uint32_t *bytes_written);
int32_t fat16_close_file(FAT_FILE *file);
int32_t fat16_delete_file(FATFS *fs, const char *path);
int32_t fat16_create_directory(FATFS *fs, const char *path);
int32_t fat16_delete_directory(FATFS *fs, const char *path);
int32_t fat16_get_directory_entries(FATFS *fs, const char *path, FAT_DIRENTRY *entries, int32_t max_entries, int32_t *entries_read);

/*
	@brief 将长文件名转换为 8.3 格式。
	@param filename 输入文件名
	@param short_name 输出的 8 字符主文件名
	@param extension 输出的 3 字符扩展名
*/
void fatfs_convert_to_83(const char *filename, char *short_name, char *extension)
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
void fatfs_convert_from_83(const char *short_name, const char *extension, char *filename)
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

/*
	@brief 初始化 FAT 文件系统。
	@param fs 文件系统上下文
	@param device 文件系统所在物理块设备
	@param type 文件系统类型（`FAT_TYPE_NONE` 表示自动检测）
	@return 错误码
*/
int32_t fatfs_init(FATFS *fs, BLKDEV *device, FAT_TYPE type)
{
	int32_t ret = BD_SUCCESS;

	/* 参数检查 */
	if (fs == NULL || device == NULL) {
		debug("FATFS: Invalid parameters in fatfs_init.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 清除文件系统上下文 */
	memset(fs, 0, sizeof(FATFS));
	fs->device = device;

	/* 根据指定类型初始化 */
	if (type == FAT_TYPE_16) {
		debug("FATFS: Initializing as FAT16.\n");
		ret = fat16_init(fs, device);
		if (ret == FATFS_SUCCESS) {
			fs->type = FAT_TYPE_16;
			return FATFS_SUCCESS;
		}
	} else if (type == FAT_TYPE_12) {
		debug("FATFS: Initializing as FAT12.\n");
		ret = fat12_init(fs, device);
		if (ret == FATFS_SUCCESS) {
			fs->type = FAT_TYPE_12;
			return FATFS_SUCCESS;
		}
	} else if (type == FAT_TYPE_NONE) {
		/* 自动检测 */
		debug("FATFS: Auto-detecting filesystem type.\n");

		/* 尝试 FAT16 */
		ret = fat16_init(fs, device);
		if (ret == FATFS_SUCCESS) {
			fs->type = FAT_TYPE_16;
			debug("FATFS: Auto-detected FAT16 filesystem.\n");
			return FATFS_SUCCESS;
		}

		/* 尝试 FAT12 */
		ret = fat12_init(fs, device);
		if (ret == FATFS_SUCCESS) {
			fs->type = FAT_TYPE_12;
			debug("FATFS: Auto-detected FAT12 filesystem.\n");
			return FATFS_SUCCESS;
		}
	}

	debug("FATFS: Failed to initialize any FAT filesystem.\n");
	fs->type = FAT_TYPE_NONE;
	return ret;
}

/*
	@brief 关闭 FAT 文件系统。
	@param fs 文件系统上下文
	@return 错误码
*/
int32_t fatfs_close(FATFS *fs)
{
	/* 参数检查 */
	if (fs == NULL) {
		debug("FATFS: Invalid parameters in fatfs_close.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (fs->type) {
		case FAT_TYPE_12:
			return fat12_close(fs);

		case FAT_TYPE_16:
			return fat16_close(fs);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized or already closed.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 打开文件。
	@param file 文件句柄
	@param fs 文件系统上下文
	@param path 文件路径
	@return 错误码
*/
int32_t fatfs_open_file(FAT_FILE *file, FATFS *fs, const char *path)
{
	/* 参数检查 */
	if (file == NULL || fs == NULL || path == NULL) {
		debug("FATFS: Invalid parameters in fatfs_open_file.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (fs->type) {
		case FAT_TYPE_12:
			return fat12_open_file(file, fs, path);

		case FAT_TYPE_16:
			return fat16_open_file(file, fs, path);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 创建文件。
	@param file 返回的文件句柄
	@param fs 文件系统上下文
	@param path 文件路径
	@return 错误码
*/
int32_t fatfs_create_file(FAT_FILE *file, FATFS *fs, const char *path)
{
	/* 参数检查 */
	if (fs == NULL || path == NULL) {
		debug("FATFS: Invalid parameters in fatfs_create_file.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (fs->type) {
		case FAT_TYPE_12:
			return fat12_create_file(file, fs, path);

		case FAT_TYPE_16:
			return fat16_create_file(file, fs, path);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 从文件读取数据。
	@param file 文件句柄
	@param buffer 数据缓冲区
	@param size 要读取的字节数
	@param bytes_read 返回实际读取的字节数
	@return 错误码
*/
int32_t fatfs_read_file(FAT_FILE *file, void *buffer, uint32_t size, uint32_t *bytes_read)
{
	/* 参数检查 */
	if (file == NULL || file->fs == NULL) {
		debug("FATFS: Invalid parameters in fatfs_read_file.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (file->fs->type) {
		case FAT_TYPE_12:
			return fat12_read_file(file, buffer, size, bytes_read);

		case FAT_TYPE_16:
			return fat16_read_file(file, buffer, size, bytes_read);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
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
int32_t fatfs_write_file(FAT_FILE *file, const void *buffer, uint32_t size, bool append, uint32_t *bytes_written)
{
	/* 参数检查 */
	if (file == NULL || file->fs == NULL) {
		debug("FATFS: Invalid parameters in fatfs_write_file.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (file->fs->type) {
		case FAT_TYPE_12:
			return fat12_write_file(file, buffer, size, append, bytes_written);

		case FAT_TYPE_16:
			return fat16_write_file(file, buffer, size, append, bytes_written);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 关闭文件。
	@param file 文件句柄
	@return 错误码
*/
int32_t fatfs_close_file(FAT_FILE *file)
{
	/* 参数检查 */
	if (file == NULL || file->fs == NULL) {
		debug("FATFS: Invalid parameters in fatfs_close_file.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (file->fs->type) {
		case FAT_TYPE_12:
			return fat12_close_file(file);

		case FAT_TYPE_16:
			return fat16_close_file(file);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 删除文件。
	@param fs 文件系统上下文
	@param path 文件路径
	@return 错误码
*/
int32_t fatfs_delete_file(FATFS *fs, const char *path)
{
	/* 参数检查 */
	if (fs == NULL || path == NULL) {
		debug("FATFS: Invalid parameters in fatfs_delete_file.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (fs->type) {
		case FAT_TYPE_12:
			return fat12_delete_file(fs, path);

		case FAT_TYPE_16:
			return fat16_delete_file(fs, path);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 创建目录。
	@param fs 文件系统上下文
	@param path 目录路径
	@return 错误码
*/
int32_t fatfs_create_directory(FATFS *fs, const char *path)
{
	/* 参数检查 */
	if (fs == NULL || path == NULL) {
		debug("FATFS: Invalid parameters in fatfs_create_directory.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (fs->type) {
		case FAT_TYPE_12:
			return fat12_create_directory(fs, path);

		case FAT_TYPE_16:
			return fat16_create_directory(fs, path);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 删除目录。
	@param fs 文件系统上下文
	@param path 目录路径
	@return 错误码
*/
int32_t fatfs_delete_directory(FATFS *fs, const char *path)
{
	if (fs == NULL || path == NULL) {
		debug("FATFS: Invalid parameters in fatfs_delete_directory.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 根据文件系统类型调用相应的删除函数 */
	switch (fs->type) {
		case FAT_TYPE_12:
			return fat12_delete_directory(fs, path);

		case FAT_TYPE_16:
			return fat16_delete_directory(fs, path);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
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
int32_t fatfs_get_directory_entries(FATFS *fs, const char *path, FAT_DIRENTRY *entries, int32_t max_entries, int32_t *entries_read)
{
	/* 参数检查 */
	if (fs == NULL || path == NULL || entries == NULL || max_entries <= 0 || entries_read == NULL) {
		debug("FATFS: Invalid parameters in fatfs_get_directory_entries.\n");
		return FATFS_INVALID_PARAM;
	}

	switch (fs->type) {
		case FAT_TYPE_12:
			return fat12_get_directory_entries(fs, path, entries, max_entries, entries_read);

		case FAT_TYPE_16:
			return fat16_get_directory_entries(fs, path, entries, max_entries, entries_read);

		case FAT_TYPE_NONE:
		default:
			debug("FATFS: Filesystem not initialized.\n");
			return FATFS_INVALID_PARAM;
	}
}

/*
	@brief 获取文件系统类型名称。
	@param type 文件系统类型
	@return 类型名称字符串
*/
const char *fatfs_get_type_name(FAT_TYPE type)
{
	switch (type) {
		case FAT_TYPE_12:
			return "FAT12";
		case FAT_TYPE_16:
			return "FAT16";
		case FAT_TYPE_NONE:
			return "NONE";
		default:
			return "UNKNOWN";
	}
}

/*
	@brief 获取文件属性名称。
	@param attributes 文件属性
	@param buffer 输出缓冲区
	@param size 缓冲区大小
*/
void fatfs_get_attribute_names(uint8_t attributes, char *buffer, size_t size)
{
	if (buffer == NULL || size == 0)
		return;

	memset(buffer, 0, size);

	if (attributes & FAT_ATTR_READONLY)
		strncat(buffer, "R/", size - strlen(buffer) - 1);
	if (attributes & FAT_ATTR_HIDDEN)
		strncat(buffer, "H/", size - strlen(buffer) - 1);
	if (attributes & FAT_ATTR_SYSTEM)
		strncat(buffer, "S/", size - strlen(buffer) - 1);
	if (attributes & FAT_ATTR_VOLUME_ID)
		strncat(buffer, "V/", size - strlen(buffer) - 1);
	if (attributes & FAT_ATTR_DIRECTORY)
		strncat(buffer, "D/", size - strlen(buffer) - 1);
	if (attributes & FAT_ATTR_ARCHIVE)
		strncat(buffer, "A/", size - strlen(buffer) - 1);

	/* 移除最后的斜杠 */
	size_t len = strlen(buffer);
	if (len > 0 && buffer[len - 1] == '/')
		buffer[len - 1] = '\0';
}

/*
	@brief 获取文件系统信息。
	@param fs 文件系统上下文
	@param total_clusters 返回总簇数
	@param free_clusters 返回空闲簇数
	@param bytes_per_cluster 返回每簇字节数
	@return 错误码
*/
int32_t fatfs_get_fs_info(FATFS *fs, uint32_t *total_clusters, uint32_t *free_clusters, uint32_t *bytes_per_cluster)
{
	/* 参数检查 */
	if (fs == NULL) {
		debug("FATFS: Invalid parameters in fatfs_get_fs_info.\n");
		return FATFS_INVALID_PARAM;
	}

	/* 计算每簇字节数 */
	uint32_t cluster_bytes = fs->bpb.bytes_per_sector * fs->bpb.sectors_per_cluster;
	if (bytes_per_cluster != NULL)
		*bytes_per_cluster = cluster_bytes;

	/* 计算总簇数 */
	uint32_t total_sectors;
	if (fs->bpb.total_sectors_short != 0)
		total_sectors = fs->bpb.total_sectors_short;
	else
		total_sectors = fs->bpb.total_sectors_long;

	uint32_t fat_sectors = fs->bpb.fat_count * fs->bpb.sectors_per_fat;
	uint32_t root_dir_sectors = ((fs->bpb.root_entries * sizeof(FAT_DIRENTRY)) + fs->bpb.bytes_per_sector - 1) / fs->bpb.bytes_per_sector;
	uint32_t data_sectors = total_sectors - (fs->bpb.reserved_sectors + fat_sectors + root_dir_sectors);
	uint32_t clusters = data_sectors / fs->bpb.sectors_per_cluster;

	if (total_clusters != NULL)
		*total_clusters = clusters;

	/* 计算空闲簇数 */
	if (free_clusters != NULL) {
		uint32_t free_count = 0;

		/* 根据文件系统类型使用不同的方法计算空闲簇 */
		switch (fs->type) {
		case FAT_TYPE_12: {
			for (uint16_t cluster = 2; cluster < clusters + 2; cluster++) {
				uint16_t next_cluster;
				if (fat12_get_next_cluster(fs, cluster, &next_cluster) == FATFS_SUCCESS) {
					if (next_cluster == 0x000)
						free_count++;
				}
			}
			break;
		}
		case FAT_TYPE_16: {
			for (uint16_t cluster = 2; cluster < clusters + 2; cluster++) {
				uint16_t next_cluster;
				if (fat16_get_next_cluster(fs, cluster, &next_cluster) == FATFS_SUCCESS) {
					if (next_cluster == 0x0000)
						free_count++;
				}
			}
			break;
		}
		default:
			free_count = 0;
			break;
		}

		*free_clusters = free_count;
	}

	return FATFS_SUCCESS;
}
