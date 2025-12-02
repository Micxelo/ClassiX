# 硬盘 - ClassiX 文档

> 当前位置: os/arch/blkdev/devices/hd.md

## 概述

保护模式中，无法使用 INT 13H 中断，需要通过 IDE 控制器完成硬盘读写操作，使用 PIO 模式。

## IDE 控制器

IDE 控制器有两个通道：主通道（Primary）和从通道（Secondary），每个通道可连接两个设备：主设备（Master）和从设备（Slave）。

### IDE 寄存器

主通道基地址 `0x1F0`  
从通道基地址 `0x170`

|Port Offset|Read/Write|Register|Description|
|:-:|:-:|:-:|:-:|
|`+0`|Read/Write|Data Register|16 位数据端口|
|`+1`|Read|Error Register|错误信息|
|`+1`|Write|Features Register|功能设置|
|`+2`|Read/Write|Sector Count Register|扇区计数|
|`+3`|Read/Write|LBA Low Register|LBA 0-7|
|`+4`|Read/Write|LBA Mid Register|LBA 8-15|
|`+5`|Read/Write|LBA High Register|LBA 16-23|
|`+6`|Read/Write|Drive Select Register|设备选择 + LBA 24-27|
|`+7`|Read|Status Register|状态寄存器|
|`+7`|Write|Command Register|命令寄存器|

### 状态寄存器

|Bit|Name|Description|
|:-:|:-:|:-:|
|0|`ERR`|Error occurred|
|1|`IDX`|Index mark (obsolete)|
|2|`CORR`|Corrected data (obsolete)|
|3|`DRQ`|Data Request Ready|
|4|`SRV`|Service request|
|5|`DF`|Device Fault|
|6|`RDY`|Device Ready|
|7|`BSY`|Device Busy|

### 设备选择寄存器

|Bit(s)|Name|Description|
|:-:|:-:|:-:|
|0-3|`LBA24-27`|LBA 地址高 4 位（28 位 LBA 模式）|
|4|`DRV`|Drive select: 0=Master; 1=Slave|
|5|`LBA`|LBA mode: 0=CHS; 1=LBA|
|6|`1`|Always 1|
|7|`1`|Always 1|

典型值：
- 主盘 LBA 模式：`0xE0` (`11100000b`)
- 从盘 LBA 模式：`0xF0` (`11110000b`)

### IDE 命令

#### 识别设备（`IDE_CMD_IDENTIFY` - `0xEC`）

获取设备详细信息，包括型号、序列号、容量、支持特性等。

**命令序列**

1. 选择设备：写入设备选择寄存器
2. 发送命令：写入命令寄存器 `0xEC`
3. 等待数据就绪：检查状态寄存器 `DRQ` 位
4. 读取数据：从数据寄存器读取 256 字（512 字节）

**IDENTIFY 设备数据结构**

|Word Offset|Description|
|:-:|:-:|
|0|General configuration|
|1|Number of cylinders|
|3|Number of heads|
|6|Number of sectors per track|
|10-19|Serial number (20 ASCII characters)|
|23-26|Firmware revision (8 ASCII characters)|
|27-46|Model number (40 ASCII characters)|
|49|Capabilities|
|60-61|User addressable sectors (LBA28)|
|83|LBA48 support and other features|
|100-103|User addressable sectors (LBA48)|

#### PIO 模式读扇区（`IDE_CMD_READ_PIO` - `0x20`）

**命令序列**

|Step|Register|Value|Description|
|:-:|:-:|:-:|:-:|
|1|Drive Select|`0xE0 + (drive << 4) + (lba >> 24)`|选择设备 + LBA 高 4 位|
|2|LBA Low|`lba & 0xFF`|LBA 0-7|
|3|LBA Mid|`(lba >> 8) & 0xFF`|LBA 8-15|
|4|LBA High|`(lba >> 16) & 0xFF`|LBA 16-23|
|5|Sector Count|`sector_count`|读取扇区数|
|6|Command|`0x20`|读扇区命令|
|7|Status|Wait for `DRQ`|等待数据就绪|
|8|Data|Read 256 words|读取扇区数据|

#### PIO 模式写扇区（`IDE_CMD_WRITE_PIO` - `0x30`）

**命令序列**

|Step|Register|Value|Description|
|:-:|:-:|:-:|:-:|
|1|Drive Select|`0xE0 + (drive << 4) + (lba >> 24)`|选择设备 + LBA 高 4 位|
|2|LBA Low|`lba & 0xFF`|LBA 0-7|
|3|LBA Mid|`(lba >> 8) & 0xFF`|LBA 8-15|
|4|LBA High|`(lba >> 16) & 0xFF`|LBA 16-23|
|5|Sector Count|`sector_count`|写入扇区数|
|6|Command|`0x30`|写扇区命令|
|7|Status|Wait for `DRQ`|等待数据就绪|
|8|Data|Write 256 words|写入扇区数据|

#### 刷新缓存（`IDE_CMD_FLUSH_CACHE` - `0xE7`）

将缓存中的数据写入磁盘。

**命令序列**

|Step|Register|Value|Description|
|:-:|:-:|:-:|:-:|
|1|Command|`0xE7`|刷新缓存命令|
|2|Status|Wait for `!BSY`|等待命令完成|

### 错误处理

错误寄存器（Error Register）位定义：

|Bit|Name|Description|
|:-:|:-:|:-:|
|0|`AMNF`|Address Mark Not Found|
|1|`TKZNF`|Track Zero Not Found|
|2|`ABRT`|Aborted Command|
|3|`MCR`|Media Change Request|
|4|`IDNF`|ID Not Found|
|5|`MC`|Media Changed|
|6|`UNC`|Uncorrectable Data Error|
|7|`BBK`|Bad Block Detected|

## 设备检测流程

1. **选择设备**：通过设备选择寄存器选择主/从设备
2. **发送识别命令**：发送 `IDENTIFY` 命令 (`0xEC`)
3. **等待响应**：检查状态寄存器，等待设备就绪
4. **读取设备信息**：从数据寄存器读取 512 字节设备信息
5. **解析信息**：提取型号、序列号、容量、支持特性等信息

## LBA 寻址模式

### LBA28

- 28 位地址，最大支持 128GB
- 通过设备选择寄存器的低 4 位提供 LBA24-27
- 兼容性较好，所有 IDE 设备都支持

### LBA48

- 48 位地址，最大支持 128PB
- 需要设备支持 LBA48 特性
- 通过两次设置寄存器来提供完整 48 位地址

## 错误码

|Code|Description|
|:-:|:-:|
|`BD_SUCCESS`|Success|
|`BD_NOT_EXIST`|Device does not exist|
|`BD_INVALID_PARAM`|Invalid parameter|
|`BD_TIMEOUT`|Operation timeout|
|`BD_NOT_READY`|Device not ready|
|`BD_IO_ERROR`|I/O error occurred|
|`BD_READONLY`|Device is read-only|