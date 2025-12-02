# 图层管理 - ClassiX 文档

> 当前位置: os/arch/ui/layer.md

## 概述

图层管理子系统提供多层图形界面管理功能，支持图层的创建、移动、Z 序调整和刷新显示，采用透明色和图层映射技术实现高效的重叠显示。

## 数据结构

### 图层管理器

|字段|类型|描述|
|:-:|:-:|:-:|
|`fb`|`uint32_t *`|帧缓冲区起始地址|
|`map`|`uint8_t *`|图层映射表，记录每个像素点所属图层|
|`width`|`uint16_t`|屏幕宽度|
|`height`|`uint16_t`|屏幕高度|
|`top`|`int32_t`|当前最高图层 Z 序|
|`layers`|`LAYER *[]`|按 Z 序排列的图层指针数组|
|`layers0`|`LAYER []`|图层对象数组|

### 图层（`LAYER`）

|字段|类型|描述|
|:-:|:-:|:-:|
|`buf`|`uint32_t *`|图层像素缓冲区|
|`width`|`uint16_t`|图层宽度|
|`height`|`uint16_t`|图层高度|
|`x`|`int16_t`|图层 X 坐标|
|`y`|`int16_t`|图层 Y 坐标|
|`z`|`int32_t`|图层 Z 序（-1 表示隐藏）|
|`flags`|`uint32_t`|图层状态标志|
|`allow_inv`|`bool`|是否启用透明色支持|

## 常量定义

|常量|值|描述|
|:-:|:-:|:-:|
|`MAX_LAYER_IDX`|`255`|最大图层下标|
|`MAX_LAYERS`|`256`|最大图层数量|
|`LAYER_FREE`|`0`|图层空闲状态|
|`LAYER_USED`|`1`|图层已使用状态|

## 全局变量

|变量|类型|描述|
|:-:|:-:|:-:|
|`layer_manager`|匿名结构体|全局图层管理器实例|

## 图层管理

### `layer_init`

初始化图层管理器。

**函数原型**

```c
int32_t layer_init(
	uint32_t *fb,
	uint16_t width, 
	uint16_t height
);
```

|参数|描述|
|:-:|:-:|
|`fb`|帧缓冲区地址|
|`width`|屏幕宽度|
|`height`|屏幕高度|

|返回值|描述|
|:-:|:-:|
|`0`|初始化成功|
|`-1`|初始化失败|

### `layer_alloc`

分配一个新的图层。

**函数原型**

```c
LAYER *layer_alloc(
	uint16_t width,
	uint16_t height,
	bool allow_inv
);
```

|参数|描述|
|:-:|:-:|
|`width`|图层宽度|
|`height`|图层高度|
|`allow_inv`|是否启用透明色|

|返回值|描述|
|:-:|:-:|
|`LAYER *`|分配的图层指针，失败返回 `NULL`|

### `layer_set_z`

设置图层的 Z 序。

**函数原型**

```c
void layer_set_z(
	LAYER *layer,
	int32_t z1
);
```

|参数|描述|
|:-:|:-:|
|`layer`|目标图层指针|
|`z1`|新的 Z 序（-1 表示隐藏）|

### `layer_refresh`

刷新图层的指定区域。

**函数原型**

```c
void layer_refresh(
	LAYER *layer,
	int32_t x0,
	int32_t y0, 
	int32_t x1,
	int32_t y1
);
```

|参数|描述|
|:-:|:-:|
|`layer`|目标图层指针|
|`x0`|区域左上角 X 坐标|
|`y0`|区域左上角 Y 坐标|
|`x1`|区域右下角 X 坐标|
|`y1`|区域右下角 Y 坐标|

### `layer_move`

移动图层到新位置。

**函数原型**

```c
void layer_move(
	LAYER *layer,
	int32_t x,
	int32_t y
);
```

|参数|描述|
|:-:|:-:|
|`layer`|目标图层指针|
|`x`|新的 X 坐标|
|`y`|新的 Y 坐标|

### `layer_free`

释放图层资源。

**函数原型**

```c
void layer_free(
	LAYER *layer
);
```

|参数|描述|
|:-:|:-:|
|`layer`|目标图层指针|

## 内部函数

### `layer_refreshmap`

刷新图层映射表，确定每个像素点显示的图层。

**函数原型**
```c
static void layer_refreshmap(
	int32_t vx0,
	int32_t vy0,
	int32_t vx1, 
	int32_t vy1,
	int32_t z0
);
```

|参数|描述|
|:-:|:-:|
|`vx0`|刷新区域左上角 X 坐标|
|`vy0`|刷新区域左上角 Y 坐标|
|`vx1`|刷新区域右下角 X 坐标|
|`vy1`|刷新区域右下角 Y 坐标|
|`z0`|起始 Z 序|

### `layer_refreshsub`

刷新指定Z序范围内的图层显示内容。

**函数原型**
```c
static void layer_refreshsub(
	int32_t vx0,
	int32_t vy0,
	int32_t vx1,
	int32_t vy1,
	int32_t z0,
	int32_t z1
);
```

|参数|描述|
|:-:|:-:|
|`vx0`|刷新区域左上角 X 坐标|
|`vy0`|刷新区域左上角 Y 坐标|
|`vx1`|刷新区域右下角 X 坐标|
|`vy1`|刷新区域右下角 Y 坐标|
|`z0`|起始 Z 序|
|`z1`|结束 Z 序|
