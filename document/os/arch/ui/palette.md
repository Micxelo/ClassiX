# 调色板 - ClassiX 文档

> 当前位置: arch/os/ui/palette.md

## 概述

调色板提供颜色定义、像素操作和 Alpha 混合功能，支持 32 位 ARGB 颜色格式，包含完整的 CGA 颜色和标准 Web 颜色定义。

## 核心功能

### 颜色定义

- 完整CGA 16 色调色板
- 标准 Web 颜色命名

### 颜色操作

- Alpha 混合计算
- 颜色反转
- 像素读写

## 数据结构

### 颜色（`COLOR`）

32 位 ARGB 颜色联合体，支持按通道访问和整体访问。

**内存布局**

```
+-----------------------+
|         color         |
|  b  |  g  |  r  |  a  |
+-----------------------+
```

## 接口

### `alpha_blend`

计算前景色与背景色的 Alpha 混合结果。

**函数原型**

```c
COLOR alpha_blend(
	COLOR fg,
	COLOR bg
);
```

|参数|描述|
|:-:|:-:|
|`fg`|前景色（支持 Alpha 通道）|
|`bg`|背景色（应完全不透明）|

|返回值|描述|
|:-:|:-:|
|`COLOR`|混合后的颜色，Alpha 通道固定为 255|

**算法说明**
- 使用快速整数运算：`(fg * alpha + bg * (257 - alpha)) >> 8`
- 要求背景色完全不透明（Alpha = 255）
- 特殊处理：前景色完全透明时返回背景色，完全不透明时返回前景色

## 颜色常量

### CGA 16 色调色板

|颜色|值|描述|
|:-:|:-:|:-:|
|`COLOR_CGA_BLACK`|`0xff000000`|CGA 黑|
|`COLOR_CGA_BLUE`|`0xff0000aa`|CGA 蓝|
|`COLOR_CGA_GREEN`|`0xff00aa00`|CGA 绿|
|`COLOR_CGA_CYAN`|`0xff00aaaa`|CGA 青|
|`COLOR_CGA_RED`|`0xffaa0000`|CGA 红|
|`COLOR_CGA_MAGENTA`|`0xffaa00aa`|CGA 品红|
|`COLOR_CGA_BROWN`|`0xffaa5500`|CGA 棕|
|`COLOR_CGA_LIGHTGRAY`|`0xffaaaaaa`|CGA 浅灰|
|`COLOR_CGA_DARKGRAY`|`0xff555555`|CGA 暗灰|
|`COLOR_CGA_LIGHTBLUE`|`0xff5555ff`|CGA 浅蓝|
|`COLOR_CGA_LIGHTGREEN`|`0xff55ff55`|CGA 浅绿|
|`COLOR_CGA_LIGHTCYAN`|`0xff55ffff`|CGA 浅青|
|`COLOR_CGA_LIGHTRED`|`0xffff5555`|CGA 浅红|
|`COLOR_CGA_LIGHTMAGENTA`|`0xffff55ff`|CGA 浅品红|
|`COLOR_CGA_YELLOW`|`0xffffff55`|CGA 黄|
|`COLOR_CGA_WHITE`|`0xffffffff`|CGA 白|

### 常用标准颜色

|颜色分类|示例|
|:-:|:-:|
|基础色|`COLOR_RED`, `COLOR_GREEN`, `COLOR_BLUE`|
|灰度色|`COLOR_BLACK`, `COLOR_WHITE`, `COLOR_GRAY`|
|Web 标准色|`COLOR_ALICEBLUE`, `COLOR_ANTIQUEWHITE`|
|特殊色|`COLOR_TRANSPARENT`, `COLOR_MIKU`|

## 宏定义

### 颜色构造

|宏|描述|
|:-:|:-:|
|`COLOR32(color)`|从 32 位整数值构造颜色|
|`COLOR32_FROM_ARGB(r, g, b, a)`|从 ARGB 分量构造颜色|
|`COLOR32_INVERT(color)`|颜色反相（保持 Alpha 通道）|

### 像素操作

|宏|描述|
|:-:|:-:|
|`SET_PIXEL32(buf, bx, x, y, c)`|设置 32 位像素值|
|`GET_PIXEL32(buf, bx, x, y)`|获取 32 位像素值|
