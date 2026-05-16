# CRC - ClassiX 文档

> 当前位置: arch/utilities/crc.md

## 概述

CRC（循环冗余校验）算法提供一种高效的错误检测机制，用于验证数据完整性。ClassiX 使用查表法实现 CRC 算法，支持对内存块进行 CRC 校验。

## 接口

### `crc8`

计算 CRC8 校验码。

**函数原型**

```c
uint8_t crc8(
	const void *buf,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`buf`|待计算的缓冲区|
|`size`|待计算的字节数|

|返回值|描述|
|:-:|:-:|
|`uint8_t`|CRC8 校验码|

### `crc16`

计算 CRC16 校验码。

**函数原型**

```c
uint16_t crc16(
	const void *buf,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`buf`|待计算的缓冲区|
|`size`|待计算的字节数|

|返回值|描述|
|:-:|:-:|
|`uint16_t`|CRC16 校验码|

### `crc32`

计算 CRC32 校验码。

**函数原型**

```c
uint32_t crc32(
	const void *buf,
	size_t size
);
```

|参数|描述|
|:-:|:-:|
|`buf`|待计算的缓冲区|
|`size`|待计算的字节数|

|返回值|描述|
|:-:|:-:|
|`uint32_t`|CRC32 校验码|
