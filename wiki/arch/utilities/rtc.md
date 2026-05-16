# RTC - ClassiX 文档

> 当前位置: arch/utilities/rtc.md

## 概述

实时时钟（RTC，Real-Time Clock）模块提供了基于 CMOS RTC 芯片的时间读取与设置功能。该模块支持年、月、日、星期、时、分、秒的获取与设置，并提供了时间戳计算功能，用于获取自某个基准时间以来的秒数。

BCD（Binary-Coded Decimal）是一种用二进制表示十进制数的编码方式，如十进制数 45 在 BCD 中表示为 `0x45`（高四位表示十位，低四位表示个位）。RTC 芯片通常以 BCD 格式存储时间数据。

## 数据结构

### 时间表示

各函数使用 `uint32_t` 类型表示时间单位（年、月、日、时、分、秒），星期使用 0（星期日）到 6（星期六）表示。

### 宏定义

|宏|描述|
|:-:|:-:|
|`BCD_TO_HEX(n)`|将 BCD 码转换为十六进制值|
|`HEX_TO_BCD(n)`|将十六进制值转换为 BCD 码|

## 接口

### `get_hour`

获取当前小时（24小时制）。

**函数原型**

```c
uint32_t get_hour(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前小时（0-23）|

### `set_hour`

设置当前小时。

**函数原型**

```c
void set_hour(
	uint32_t hour_hex
);
```

|参数|描述|
|:-:|:-:|
|`hour_hex`|要设置的小时值（0-23）|

### `get_minute`

获取当前分钟。

**函数原型**

```c
uint32_t get_minute(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前分钟（0-59）|

### `set_minute`

设置当前分钟。

**函数原型**

```c
void set_minute(
	uint32_t min_hex
);
```

|参数|描述|
|:-:|:-:|
|`min_hex`|要设置的分钟值（0-59）|

### `get_second`

获取当前秒。

**函数原型**

```c
uint32_t get_second(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前秒（0-59）|

### `set_second`

设置当前秒。

**函数原型**

```c
void set_second(
	uint32_t sec_hex
);
```

|参数|描述|
|:-:|:-:|
|`sec_hex`|要设置的秒值（0-59）|

### `get_day_of_month`

获取当前日期（月中的天）。

**函数原型**

```c
uint32_t get_day_of_month(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前日期（1-31）|

### `set_day_of_month`

设置当前日期（月中的天）。

**函数原型**

```c
void set_day_of_month(
	uint32_t day_of_month
);
```

|参数|描述|
|:-:|:-:|
|`day_of_month`|要设置的日期值（1-31）|

### `get_day_of_week`

获取当前星期。

**函数原型**

```c
uint32_t get_day_of_week(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前星期（0-6，0 表示星期日）

### `set_day_of_week`

设置当前星期。

**函数原型**

```c
void set_day_of_week(
	uint32_t day_of_week
);
```

|参数|描述|
|:-:|:-:|
|`day_of_week`|要设置的星期值（0-6，0 表示星期日）|

### `get_month`

获取当前月份。

**函数原型**

```c
uint32_t get_month(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前月份（1-12）|

### `set_month`

设置当前月份。

**函数原型**

```c
void set_month(
	uint32_t mon_hex
);
```

|参数|描述|
|:-:|:-:|
|`mon_hex`|要设置的月份值（1-12）|

### `get_year`

获取当前年份。

**函数原型**

```c
uint32_t get_year(void);
```

|返回值|描述|
|:-:|:-:|
|`uint32_t`|当前年份（如 2025）|

### `set_year`

设置当前年份。

**函数原型**

```c
void set_year(
	uint32_t year
);
```

|参数|描述|
|:-:|:-:|
|`year`|要设置的年份值（如 2025）|

### `timestamp`

计算给定日期时间的时间戳（秒数）。

**函数原型**

```c
uint64_t timestamp(
	int32_t year,
	int32_t mon,
	int32_t day,
	int32_t hour,
	int32_t min,
	int32_t sec
);
```

|参数|描述|
|:-:|:-:|
|`year`|年份（如 2025）|
|`mon`|月份（1-12）|
|`day`|日期（1-31）|
|`hour`|小时（0-23）|
|`min`|分钟（0-59）|
|`sec`|秒（0-59）|

|返回值|描述|
|:-:|:-:|
|`uint64_t`|自 1970-01-01 00:00:00 UTC 起的秒数（时间戳）|
