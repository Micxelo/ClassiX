# 帧缓冲区 - ClassiX 文档

> 当前位置: arch/ui/framebuf.md

## 概述

帧缓冲区管理子系统提供物理显存的初始化和像素级操作功能，支持多种颜色格式和位深度的帧缓冲区。

## 核心功能

### 帧缓冲区初始化

- 基于 Multiboot 信息的自动检测和配置
- 支持多种颜色格式识别
- 自动选择优化的像素操作函数

### 像素级操作

- 像素颜色读取和设置
- 支持 8/15/16/24/32 位色深
- 颜色分量掩码处理
- 颜色精度扩展

### 颜色格式支持

- 标准 32 位 ARGB 格式
- 通用颜色格式（RGB/BGR 等）
- 颜色掩码配置
- 位深度自适应

## 数据结构

### FRAMEBUFFER

帧缓冲区信息结构体，包含显存地址、尺寸、位深度和颜色格式等信息。

```c
typedef struct {
	uintptr_t addr;					/* 帧缓冲地址 */
	uint32_t pitch;					/* 帧缓冲行距 */
	uint32_t width;					/* 帧缓冲宽度 */
	uint32_t height;				/* 帧缓冲高度 */
	uint8_t bpp;					/* 帧缓冲位深度 */
	uint8_t red_field_position;		/* R 字段位置 */
	uint8_t red_mask_size;			/* R 掩码大小 */
	uint8_t green_field_position;	/* G 字段位置 */
	uint8_t green_mask_size;		/* G 掩码大小 */
	uint8_t blue_field_position;	/* B 字段位置 */
	uint8_t blue_mask_size;			/* B 掩码大小 */
	bool argb_format;				/* 标准 ARGB 颜色格式 */
} FRAMEBUFFER;
```

## 全局变量

### `g_fb`

全局帧缓冲区信息结构体，存储当前帧缓冲区的所有配置信息。

**类型**

```c
extern FRAMEBUFFER g_fb;
```

## 接口

### `init_framebuffer`

初始化全局帧缓冲区。

**函数原型**

```c
int32_t init_framebuffer(
	const multiboot_info_t *mbi
);
```

|参数|描述|
|:-:|:-:|
|`mbi`|Multiboot 信息指针|

**返回值**

|值|描述|
|:-:|:-:|
|`0`|初始化成功|
|`-1`|初始化失败|

**说明**

- 自动检测帧缓冲区信息并配置全局 `g_fb` 结构
- 识别标准 ARGB 格式并选择优化的像素操作函数
- 对于非标准格式使用通用像素操作函数

### `get_pixel`

获取物理显存指定像素的颜色值。

**函数原型**

```c
COLOR get_pixel(
	uint16_t x,
	uint16_t y
);
```

|参数|描述|
|:-:|:-:|
|`x`|X 坐标|
|`y`|Y 坐标|

**返回值**

|值|描述|
|:-:|:-:|
|`COLOR`|该点的颜色值|

**说明**

- 自动根据帧缓冲区格式选择相应的像素读取函数
- 超出帧缓冲范围时返回黑色

### `set_pixel`

设置物理显存指定像素的颜色值。

**函数原型**

```c
void set_pixel(
	uint16_t x,
	uint16_t y, 
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`x`|X 坐标|
|`y`|Y 坐标|
|`color`|指定的颜色值|

**说明**

- 自动根据帧缓冲区格式选择相应的像素设置函数
- 超出帧缓冲范围时无操作

## 内部函数

### `get_pixel_argb`

适用于 ARGB 颜色格式缓冲区的像素读取。

**函数原型**

```c
static inline COLOR get_pixel_argb(
	uint16_t x,
	uint16_t y
);
```

### `set_pixel_argb`

适用于 ARGB 颜色格式缓冲区的像素设置。

**函数原型**

```c
static inline void set_pixel_argb(
	uint16_t x,
	uint16_t y,
	COLOR color
);
```

### `get_pixel_universal`

适用于通用颜色格式缓冲区的像素读取。

**函数原型**

```c
static inline COLOR get_pixel_universal(
	uint16_t x,
	uint16_t y
);
```

### `set_pixel_universal`

适用于通用颜色格式缓冲区的像素设置。

**函数原型**

```c
static inline void set_pixel_universal(
	uint16_t x,
	uint16_t y,
	COLOR color
);
```

### `expand_to_8bit`

将颜色分量扩展到 8 位精度。

**函数原型**

```c
static inline uint8_t expand_to_8bit(
	uint32_t value,
	uint8_t src_bits
);
```

|参数|描述|
|:-:|:-:|
|`value`|原始颜色分量值|
|`src_bits`|原始位深度|

**返回值**

|值|描述|
|:-:|:-:|
|`uint8_t`|扩展后的 8 位颜色值|

**说明**

- 使用线性扩展算法：`value * 255 / (2 ^ src_bits - 1)`
- 支持任意位深度的颜色分量扩展

## 颜色格式处理

### 标准 ARGB 格式

- 位深度：32 位
- 红色字段位置：16
- 绿色字段位置：8  
- 蓝色字段位置：0
- 使用优化的直接像素操作

### 通用颜色格式

- 支持任意颜色字段位置和掩码大小
- 自动处理颜色分量提取和组合
- 支持颜色精度扩展
- 适用于各种非标准颜色格式

## 性能优化

- 对标准 ARGB 格式使用内联优化函数
- 自动选择最优的像素操作路径
- 减少通用格式下的计算开销
- 支持快速像素地址计算
