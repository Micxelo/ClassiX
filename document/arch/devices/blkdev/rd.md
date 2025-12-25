# RAM 磁盘驱动 - ClassiX 文档

> 当前位置：os/arch/devices/blkdev/rd.md

## 概述

RAM 磁盘（RAM Disk）是一种将内存空间模拟为块设备的驱动实现。它将连续的内存区域作为存储介质，提供与物理磁盘相同的读写接口，但具有内存级别的访问速度。RAM 磁盘常用于临时存储、缓存。

## 接口

### `rd_init`

初始化一个 RAM 磁盘设备，设置其参数并注册读写函数。

**函数原型**

```c
int32_t rd_init(
	uint8_t *buf,
	uint32_t bytes_per_sector,
	uint32_t total_sectors,
	BLKDEV *rd_dev
);
```

#### 参数

|参数|描述|
|:-:|:-:|
|`buf`|RAM 磁盘的数据缓冲区指针|
|`bytes_per_sector`|每个扇区的字节数|
|`total_sectors`|RAM 磁盘的总扇区数|
|`rd_dev`|待初始化的块设备结构体指针|

## 错误码

|Code|Description|
|:-:|:-:|
|`BD_SUCCESS`|Success|
|`BD_INVALID_PARAM`|Invalid parameter|
