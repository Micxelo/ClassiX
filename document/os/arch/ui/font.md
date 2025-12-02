# 字体 - ClassiX 文档

> 当前位置：os/arch/ui/font.md

## 概述

ClassiX 支持 PSF 字体的加载、解析和渲染，包含 Unicode 字符映射功能。

## PSF 字体格式

PSF（PC Screen Font）是 Linux 控制台使用的位图字体格式，广泛用于操作系统内核和嵌入式系统的文本渲染。ClassiX 支持 PSF1 和 PSF2 两种版本。

### 格式特性

- **位图字体**：每个字符以位图形式存储
- **固定尺寸**：所有字符具有相同宽度和高度
- **Unicode 支持**：可选 Unicode 字符映射表
- **高效渲染**：直接位操作，无复杂解码

### PSF1

#### 文件头结构
```c
typedef struct __attribute__((packed)) {
	uint16_t magic;			/* 魔数 0x0436 */
	uint8_t mode;			/* 模式标志位 */
	uint8_t charsize;		/* 字符高度 */
} PSF1_HEADER;
```

#### 模式标志位

|标志位|值|描述|
|:-:|:-:|:-:|
|`PSF1_MODE512`|`0x01`|512 字符模式（默认 256）|
|`PSF1_MODEHASTAB`|`0x02`|包含 Unicode 映射表|
|`PSF1_MODEHASSEQ`|`0x04`|包含字符序列|
|`PSF1_MAXMODE`|`0x05`|最大模式值|

### PSF2 格式

#### 文件头结构
```c
typedef struct __attribute__((packed)) {
	uint32_t magic;			/* 魔数 0x864ab572 */
	uint32_t version;		/* 版本号（应为 0） */
	uint32_t headersize;	/* Bitmap 偏移量 */
	uint32_t flags;			/* 特征标志 */
	uint32_t length;		/* 字形数 */
	uint32_t charsize;		/* 每个字符所占字节数 */
	uint32_t height;		/* 字符高度 */
	uint32_t width;			/* 字符宽度 */
} PSF2_HEADER;
```

#### 特征标志
|标志位|值|描述|
|:-:|:-:|:-:|
|`PSF2_HAS_UNICODE_TABLE`|`0x01`|包含 Unicode 映射表|

### 字形数据

每个字符的字形数据按行存储，每行按位编码：

对于 8×16 字体，每个字符占用 16 字节，每个字节编码字符的一行像素。

### Unicode 映射表

Unicode 表为每个字形提供 Unicode 码点映射：

- **PSF1**：使用 `0xFF` 作为字符分隔符
- **PSF2**：使用 `0xFF` 作为分隔符，`0xFE` 作为序列开始标记
- 支持 UTF-8 编码的 Unicode 字符序列

## 数据结构

### 字体结构（`BITMAP_FONT`）

|字段|类型|描述|
|:-:|:-:|:-:|
|`width`|`uint32_t`|字符宽度（像素）|
|`height`|`uint32_t`|字符高度（像素）|
|`charsize`|`uint32_t`|单个字符所占字节数|
|`count`|`uint32_t`|字符数量|
|`buf`|`uint8_t *`|字体位图数据指针|
|`has_unicode`|`bool`|是否包含 Unicode 映射表|
|`unicode_map`|`uint32_t *`|Unicode 映射表指针|
|`version`|`uint8_t`|PSF 版本（1 或 2）|

## 常量定义

### 魔数标识

|常量|值|描述|
|:-:|:-:|:-:|
|`PSF1_MAGIC`|`0x0436`|PSF1 格式魔数|
|`PSF2_MAGIC`|`0x864ab572`|PSF2 格式魔数|

### Unicode 表标记

|常量|值|描述|
|:-:|:-:|:-:|
|`PSF1_SEPARATOR`|`0xffff`|PSF1 Unicode 表分隔符|
|`PSF1_STARTSEQ`|`0xfffe`|PSF1 序列开始标记|
|`PSF2_SEPARATOR`|`0xff`|PSF2 Unicode 表分隔符|
|`PSF2_STARTSEQ`|`0xfe`|PSF2 序列开始标记|

## 接口

### `psf_load()`

从内存缓冲区加载 PSF 字体。

**函数原型**

```c
BITMAP_FONT psf_load(
	const uint8_t *buf,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`buf`|字体文件内存缓冲区|
|`size`|字体文件大小（字节）|

|返回值|描述|
|:-:|:-:|
|`BITMAP_FONT`|加载的字体对象，失败返回空结构|

**功能说明：**
- 自动检测 PSF1 或 PSF2 格式
- 验证文件完整性和魔数
- 解析字体头信息和字形数据
- 可选解析 Unicode 映射表
- 分配内存存储字体数据

### `psf_free()`

释放字体资源。

**函数原型**

```c
void psf_free(
	BITMAP_FONT *font
);
```

|参数|描述|
|:-:|:-:|
|`font`|要释放的字体对象指针|

**说明：**
- 释放字形数据缓冲区
- 释放 Unicode 映射表
- 将指针置为 NULL

### `psf_find_char_index()`

查找 Unicode 字符对应的字体索引。

**函数原型**

```c
uint32_t psf_find_char_index(
	const BITMAP_FONT *font,
	uint32_t unicode
);
```

|参数|描述|
|:-:|:-:|
|`font`|字体对象指针|
|`unicode`|Unicode 码点|

|返回值|描述|
|:-:|:-:|
|`uint32_t`|字符索引，找不到时返回 0|

**查找策略：**
1. 使用 Unicode 映射表精确匹配
2. 对于 ASCII 字符（<128），直接使用码点作为索引
3. 特殊字符（如 U+FFFD 替换字符）映射到默认字符
4. 所有查找失败时返回索引 0

## 内部函数

### `psf_validator()`

验证内存中的 PSF 字体文件。

**函数原型**

```c
static uint32_t psf_validator(
	const uint8_t *buf
);
```

|参数|描述|
|:-:|:-:|
|`buf`|字体文件内存缓冲区|

|返回值|描述|
|:-:|:-:|
|`uint32_t`|PSF 版本号（1、2），0 表示无效|

### `parse_psf1_unicode_table()`

解析 PSF1 Unicode 表。

**函数原型**

```c
static void parse_psf1_unicode_table(
	const BITMAP_FONT *font,
	const uint8_t *table_data,
	uint32_t table_size
);
```

### `parse_psf2_unicode_table()`

解析 PSF2 Unicode 表。

**函数原型**

```c
static void parse_psf2_unicode_table(
	const BITMAP_FONT *font,
	const uint8_t *table_data,
	uint32_t table_size
);
```

## 依赖关系

### 头文件
- `ClassiX/font.h` - 字体结构定义和函数声明
- `ClassiX/memory.h` - 内存分配函数（kmalloc/kfree）
- `ClassiX/typedef.h` - 基本类型定义
- `string.h` - 内存操作函数

### 内存管理
- 使用 `kmalloc()` 分配字体数据缓冲区
- 使用 `kfree()` 释放字体资源
- 自动处理内存分配失败情况

## 错误处理

### 文件验证
- 检查魔数有效性
- 验证文件大小是否足够容纳头部
- 检查数据完整性

### 内存分配
- 分配失败时返回空字体对象
- 部分分配失败时释放已分配资源
- Unicode 表解析错误不影响基本字体功能

## 兼容性说明

### 格式支持
- **PSF1**：固定 8 像素宽度，支持 256/512 字符
- **PSF2**：可变宽度，支持更多字符和特性

### 字符集支持
- 基本 ASCII 字符集（0-127）
- 扩展 ASCII 字符集（128-255）
- Unicode 字符（通过映射表）
- 自动回退到默认字符