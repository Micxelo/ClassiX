# CGA 文本显示 - ClassiX 文档

> 当前位置: arch/ui/cga.md

## 概述

CGA 文本显示子系统提供 CGA（Color Graphics Adapter）兼容的文本模式显示支持，实现 80×25 字符文本终端的完整功能，包括光标控制、颜色设置、滚屏和格式化输出。

## 技术规格

### 显示模式

- **分辨率**：80 列 × 25 行
- **字符集**：代码页 437（扩展 ASCII）
- **颜色**：16 色前景，8 色背景
- **缓冲区地址**：`0xB8000`

### 字符属性

每个字符由两个字节组成：

- **低字节**：ASCII 字符代码
- **高字节**：颜色属性（前景色 + 背景色）

## 常量定义

### 显示参数

|常量|值|描述|
|:-:|:-:|:-:|
|`CGA_COLUMNS`|`80`|文本列数|
|`CGA_ROWS`|`25`|文本行数|
|`CGA_BUF_ADDR`|`0xB8000`|显存缓冲区地址|
|`CGA_SIZE`|`2000`|缓冲区总字符数（80×25）|

### 颜色定义

|颜色|值|描述|
|:-:|:-:|:-:|
|`CGA_BLACK`|`0x0`|黑色|
|`CGA_BLUE`|`0x1`|蓝色|
|`CGA_GREEN`|`0x2`|绿色|
|`CGA_CYAN`|`0x3`|青色|
|`CGA_RED`|`0x4`|红色|
|`CGA_MAGENTA`|`0x5`|洋红色|
|`CGA_BROWN`|`0x6`|棕色|
|`CGA_LIGHTGRAY`|`0x7`|浅灰色|
|`CGA_DARKGRAY`|`0x8`|深灰色|
|`CGA_LIGHTBLUE`|`0x9`|浅蓝色|
|`CGA_LIGHTGREEN`|`0xa`|浅绿色|
|`CGA_LIGHTCYAN`|`0xb`|浅青色|
|`CGA_LIGHTRED`|`0xc`|浅红色|
|`CGA_LIGHTMAGENTA`|`0xd`|浅洋红色|
|`CGA_YELLOW`|`0xe`|黄色|
|`CGA_WHITE`|`0xf`|白色|

### 光标控制端口

|端口|索引|描述|
|:-:|:-:|:-:|
|`0x3d4`|`0x0e`|光标位置高字节|
|`0x3d4`|`0x0f`|光标位置低字节|
|`0x3d4`|`0x0a`|光标起始扫描线|
|`0x3d4`|`0x0b`|光标结束扫描线|

## 全局变量

|变量|类型|描述|
|:-:|:-:|:-:|
|`cga_cursor_x`|`uint16_t`|当前光标列位置（0-79）|
|`cga_cursor_y`|`uint16_t`|当前光标行位置（0-24）|
|`cga_color`|`uint16_t`|当前颜色属性|
|`buffer`|`uint16_t *`|CGA 显存缓冲区指针|

## 接口

### `set_cga_cursor_pos`

设置 CGA 终端输出字符位置（逻辑位置）。

**函数原型**

```c
void set_cga_cursor_pos(
	uint16_t x,
	uint16_t y
);
```

|参数|描述|
|:-:|:-:|
|`x`|列坐标（0-79）|
|`y`|行坐标（0-24）|

### `get_cga_cursor_pos`

获取当前光标位置。

**函数原型**

```c
uint16_t get_cga_cursor_pos(void);
```

|返回值|描述|
|:-:|:-:|
|`uint16_t`|光标位置索引（x + y × 80）|

### `cga_move_cursor`

设置 CGA 终端硬件光标位置。

**函数原型**

```c
void cga_move_cursor(
	uint16_t x,
	uint16_t y
);
```

|参数|描述|
|:-:|:-:|
|`x`|列坐标（0-79）|
|`y`|行坐标（0-24）|

### `cga_hide_cursor`

隐藏 CGA 终端光标。

**函数原型**

```c
void cga_hide_cursor(void);
```

### `cga_show_cursor`

显示 CGA 终端光标。

**函数原型**

```c
void cga_show_cursor(void);
```

### `set_cga_color`

设置 CGA 终端输出字符颜色。

**函数原型**

```c
void set_cga_color(
	uint16_t color
);
```

|参数|描述|
|:-:|:-:|
|`color`|颜色属性（前景色 \| 背景色 << 4）|

### `cga_clear`

清空 CGA 终端缓冲区。

**函数原型**

```c
void cga_clear(void);
```

**效果**
- 光标重置到 (0,0)
- 整个屏幕填充空格字符
- 使用当前颜色属性

### 字符输出

### `cga_putchar`

CGA 终端打印单个字符。

**函数原型**

```c
void cga_putchar(
	char c
);
```

|参数|描述|
|:-:|:-:|
|`c`|要输出的字符|

**控制字符支持**
|字符|功能|
|:-:|:-:|
|`\b`|退格，删除前一个字符|
|`\r`|回车，光标移动到行首|
|`\n`|换行，光标移动到下一行|
|`\t`|制表符，4 字符对齐|

### `cga_puts`

CGA 终端打印字符串。

**函数原型**

```c
void cga_puts(
	const char *str
);
```

|参数|描述|
|:-:|:-:|
|`str`|要输出的字符串|

### `cga_printf`

CGA 终端格式化输出。

**函数原型**

```c
int32_t cga_printf(
	const char *format,
	...
);
```

|参数|描述|
|:-:|:-:|
|`format`|格式化字符串|
|`...`|可变参数列表|

|返回值|描述|
|:-:|:-:|
|`int32_t`|输出的字符数|

**说明**
- 支持标准 printf 格式化
- 缓冲区限制为 1024 字节
- 使用 `vsnprintf` 实现

## 内部函数

### `cga_backspace`

执行退格操作。

**函数原型**

```c
static void cga_backspace(void);
```

**行为**
- 光标左移一列
- 如果已在行首，则移动到上一行行尾
- 清除该位置的字符

### `cga_newline`

执行换行操作。

**函数原型**

```c
static void cga_newline(void);
```

**行为**
- 光标移动到下一行行首
- 如果已在最后一行，执行滚屏操作

### `cga_tab`

执行制表符操作。

**函数原型**

```c
static void cga_tab(void);
```

**行为**
- 制表符宽度：4 字符
- 自动对齐到 4 的倍数
- 如果行空间不足，自动换行

## 滚屏机制

当光标到达屏幕底部并需要换行时，系统执行滚屏：

1. 将第 2-25 行内容上移一行
2. 清除第 25 行（新行）
3. 光标定位到第 25 行行首
