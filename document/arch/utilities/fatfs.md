# FATFS - ClassiX 文档

> 当前位置: arch/utilities/fatfs.md

## 概述

FATFS 提供了对 FAT12 和 FAT16 文件系统的支持，实现了磁盘分区检测、引导扇区解析、文件与目录操作、簇链管理等核心功能，并提供了统一的接口用于文件系统操作。

## 设计原理

FAT 文件系统采用簇（Cluster）作为基本的存储单位，通过文件分配表（FAT）管理磁盘空间。文件系统由引导扇区、FAT 表、根目录区和数据区组成。引导扇区包含文件系统参数，FAT 表记录每个簇的分配状态和链接关系，根目录区存放文件和目录的元数据，数据区存储实际文件内容。

### 目录结构

FAT 文件系统采用树状目录结构，支持多级子目录。每个目录由目录项（Directory Entry）组成，每个目录项占用 32 字节，包含文件名、扩展名、属性、时间戳、起始簇号和文件大小等信息。根目录有固定的大小和位置，而子目录作为特殊文件存储在数据区中，可以动态扩展。

### 索引方式

FAT 文件系统使用簇链来索引文件数据。文件的起始簇号存储在目录项中，后续簇号通过 FAT 表链接。FAT 表是一个数组，每个表项对应一个数据簇，表项的值表示该簇的状态或下一个簇的编号。

FAT12：每个 FAT 表项占用 12 位（1.5 字节）。簇号范围 0x000-0xFFF，其中 0x000 表示空闲簇，0x001 为保留簇，0x002-0xFEF 为数据簇，0xFF0-0xFF6 为保留值，0xFF7 表示坏簇，0xFF8-0xFFF 表示文件结束。

FAT16：每个 FAT 表项占用 16 位（2 字节）。簇号范围 0x0000-0xFFFF，其中 0x0000 表示空闲簇，0x0001 为保留簇，0x0002-0xFFEF 为数据簇，0xFFF0-0xFFF6 为保留值，0xFFF7 表示坏簇，0xFFF8-0xFFFF 表示文件结束。

## 常量定义

### 分区类型

|常量|值|描述|
|:-:|:-:|:-:|
|`PART_TYPE_EMPTY`|0x00|空分区|
|`PART_TYPE_FAT12`|0x01|FAT12 分区|
|`PART_TYPE_FAT16`|0x04|FAT16 分区|
|`PART_TYPE_FAT16B`|0x06|FAT16B 分区|
|`PART_TYPE_FAT16_LBA`|0x0e|FAT16 LBA 分区|
|`PART_TYPE_FAT32`|0x0b|FAT32 分区|
|`PART_TYPE_FAT32_LBA`|0x0c|FAT32 LBA 分区|
|`PART_TYPE_NTFS`|0x07|NTFS 分区|
|`PART_TYPE_LINUX`|0x83|Linux 分区|
|`PART_TYPE_GPT`|0xee|GPT 保留分区|

### 文件系统类型

|常量|值|描述|
|:-:|:-:|:-:|
|`FAT_TYPE_NONE`|0|未识别或未初始化|
|`FAT_TYPE_12`|1|FAT12 文件系统|
|`FAT_TYPE_16`|2|FAT16 文件系统|

### 文件属性

|常量|值|描述|
|:-:|:-:|:-:|
|`FAT_ATTR_READONLY`|0x01|只读文件|
|`FAT_ATTR_HIDDEN`|0x02|隐藏文件|
|`FAT_ATTR_SYSTEM`|0x04|系统文件|
|`FAT_ATTR_VOLUME_ID`|0x08|卷标|
|`FAT_ATTR_DIRECTORY`|0x10|目录|
|`FAT_ATTR_ARCHIVE`|0x20|归档|
|`FAT_ATTR_LONGNAME`|0x0F|长文件名|

## 数据结构

### `PART_ENTRY`

分区表项结构，包含分区的起始地址、大小和类型等信息。

### `MBR`

主引导记录结构，包含分区表和引导代码等信息。

### `FAT_BPB`

BIOS 参数块结构，存储文件系统的基本参数，如每簇扇区数、FAT 表数目、根目录项数等。

### `FAT_DIRENTRY`

目录项结构，包含文件或目录的元数据，如文件名、属性、起始簇号和文件大小等。

### `FATFS`

文件系统上下文结构，维护文件系统的状态信息，包括块设备指针、FAT 表位置、数据区位置和文件系统类型等。

### `FAT_FILE`

文件句柄结构，表示打开的文件，包含当前读写位置、文件大小和所属文件系统等信息。

## 全局变量

|变量|类型|描述|
|:-:|:-:|:-:|
|`g_fs`|`FATFS * const`|全局文件系统上下文指针|

## 接口

### `fatfs_init`

初始化FAT文件系统，自动检测或指定文件系统类型。

**函数原型**

```c
int32_t fatfs_init(
	FATFS *fs,
	BLKDEV *device,
	FAT_TYPE type
);
```

|参数|描述|
|:-:|:-:|
|`fs`|文件系统上下文指针|
|`device`|块设备指针|
|`type`|文件系统类型（`FAT_TYPE_NONE` 表示自动检测）|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_close`

关闭FAT文件系统，释放相关资源。

**函数原型**

```c
int32_t fatfs_close(
	FATFS *fs
);
```

|参数|描述|
|:-:|:-:|
|`fs`|文件系统上下文指针|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_open_file`

打开指定路径的文件。

**函数原型**

```c
int32_t fatfs_open_file(
	FAT_FILE *file,
	FATFS *fs,
	const char *path
);
```

|参数|描述|
|:-:|:-:|
|`file`|文件句柄指针|
|`fs`|文件系统上下文指针|
|`path`|文件路径字符串|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_create_file`

创建新文件。

**函数原型**

```c
int32_t fatfs_create_file(
	FAT_FILE *file,
	FATFS *fs,
	const char *path
);
```

|参数|描述|
|:-:|:-:|
|`file`|返回的文件句柄指针（可为 `NULL`）|
|`fs`|文件系统上下文指针|
|`path`|文件路径字符串|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_read_file`

从打开的文件中读取数据。

**函数原型**

```c
int32_t fatfs_read_file(
	FAT_FILE *file,
	void *buffer,
	uint32_t size,
	uint32_t *bytes_read
);
```

|参数|描述|
|:-:|:-:|
|`file`|文件句柄指针|
|`buffer`|数据缓冲区指针|
|`size`|请求读取的字节数|
|`bytes_read`|返回实际读取的字节数|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_write_file`

向打开的文件写入数据。

**函数原型**

```c
int32_t fatfs_write_file(
	FAT_FILE *file,
	const void *buffer,
	uint32_t size,
	bool append,
	uint32_t *bytes_written
);
```

|参数|描述|
|:-:|:-:|
|`file`|文件句柄指针|
|`buffer`|数据缓冲区指针|
|`size`|请求写入的字节数|
|`append`|是否为追加模式|
|`bytes_written`|返回实际写入的字节数|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_close_file`

关闭打开的文件。

**函数原型**

```c
int32_t fatfs_close_file(
	FAT_FILE *file
);
```

|参数|描述|
|:-:|:-:|
|`file`|文件句柄指针|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_delete_file`

删除指定文件。

**函数原型**

```c
int32_t fatfs_delete_file(
	FATFS *fs,
	const char *path
);
```

|参数|描述|
|:-:|:-:|
|`fs`|文件系统上下文指针|
|`path`|文件路径字符串|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_create_directory`

创建新目录。

**函数原型**

```c
int32_t fatfs_create_directory(
	FATFS *fs,
	const char *path
);
```

|参数|描述|
|:-:|:-:|
|`fs`|文件系统上下文指针|
|`path`|目录路径字符串|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_delete_directory`

删除指定目录（目录必须为空）。

**函数原型**

```c
int32_t fatfs_delete_directory(
	FATFS *fs,
	const char *path
);
```

|参数|描述|
|:-:|:-:|
|`fs`|文件系统上下文指针|
|`path`|目录路径字符串|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_get_directory_entries`

获取指定目录的内容列表。

**函数原型**

```c
int32_t fatfs_get_directory_entries(
	FATFS *fs,
	const char *path,
	FAT_DIRENTRY *entries,
	int32_t max_entries,
	int32_t *entries_read
);
```

|参数|描述|
|:-:|:-:|
|`fs`|文件系统上下文指针|
|`path`|目录路径字符串|
|`entries`|目录项数组指针|
|`max_entries`|最大目录项数|
|`entries_read`|返回实际读取的目录项数|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

### `fatfs_convert_to_83`

将长文件名转换为 8.3 格式。

**函数原型**

```c
void fatfs_convert_to_83(
	const char *filename,
	char *short_name,
	char *extension
);
```

|参数|描述|
|:-:|:-:|
|`filename`|输入文件名|
|`short_name`|输出的 8 字符主文件名|
|`extension`|输出的 3 字符扩展名|

### `fatfs_convert_from_83`

将 8.3 格式转换为长文件名。

**函数原型**

```c
void fatfs_convert_from_83(
	const char *short_name,
	const char *extension,
	char *filename
);
```

|参数|描述|
|:-:|:-:|
|`short_name`|8 字符主文件名|
|`extension`|3 字符扩展名|
|`filename`|输出文件名缓冲区|

### `fatfs_get_type_name`

获取文件系统类型名称。

**函数原型**

```c
const char *fatfs_get_type_name(
	FAT_TYPE type
);
```

|参数|描述|
|:-:|:-:|
|`type`|文件系统类型枚举值|

|返回值|描述|
|:-:|:-:|
|`const char *`|类型名称字符串|

### `fatfs_get_attribute_names`

获取文件属性名称。

**函数原型**

```c
void fatfs_get_attribute_names(
	uint8_t attributes,
	char *buffer,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`attributes`|文件属性位掩码|
|`buffer`|输出缓冲区|
|`size`|缓冲区大小|

### `fatfs_get_fs_info`

获取文件系统信息。

**函数原型**

```c
int32_t fatfs_get_fs_info(
	FATFS *fs,
	uint32_t *total_clusters,
	uint32_t *free_clusters,
	uint32_t *bytes_per_cluster
);
```

|参数|描述|
|:-:|:-:|
|`fs`|文件系统上下文指针|
|`total_clusters`|返回总簇数|
|`free_clusters`|返回空闲簇数|
|`bytes_per_cluster`|返回每簇字节数|

|返回值|描述|
|:-:|:-:|
|`int32_t`|错误码|

## 错误码

|错误码|描述|
|:-:|:-:|
|`FATFS_SUCCESS`|成功|
|`FATFS_INVALID_PARAM`|非法参数|
|`FATFS_READ_FAILED`|读失败|
|`FATFS_WRITE_FAILED`|写失败|
|`FATFS_INVALID_SIGNATURE`|非法签名|
|`FATFS_INVALID_MBR`|非法 MBR 参数|
|`FATFS_INVALID_BPB`|非法 BPB 参数|
|`FATFS_NO_PARTITION`|无指定分区|
|`FATFS_MEMORY_ALLOC`|内存分配失败|
|`FATFS_NO_FREE_CLUSTER`|无可用簇|
|`FATFS_ENTRY_EXISTS`|条目已存在|
|`FATFS_ENTRY_NOT_FOUND`|条目未找到|
|`FATFS_DIR_FULL`|目录已满|
|`FATFS_IS_DIRECTORY`|指定条目为目录|
|`FATFS_DIR_NOT_EMPTY`|目录不为空|
