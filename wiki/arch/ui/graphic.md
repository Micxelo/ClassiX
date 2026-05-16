# 图形绘制 - ClassiX 文档

> 当前位置: arch/ui/graphic.md

## 概述

图形绘制子系统提供基本的 2D 图形绘制功能，包括几何图形绘制、位块传输和字体渲染，支持 32 位色深图形缓冲区操作。

## 核心功能

### 几何图形绘制

- 矩形边框和填充矩形
- 直线绘制
- 圆形和椭圆绘制
- 支持不同线宽和颜色

### 文本渲染

- PSF 字体支持
- ASCII 和 Unicode 字符串绘制
- UTF-8 编码解码

### 图像操作

- 位块传输（Bit Blit）
- 像素级操作

## 坐标系统

图形坐标系以左上角为原点 (0,0)，向右为 X 轴正方向，向下为 Y 轴正方向。

## 接口

### `draw_rectangle`

指定左上角坐标、宽度和高度绘制矩形边框。

**函数原型**

```c
void draw_rectangle(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x,
	uint16_t y,
	uint16_t width,
	uint16_t height,
	uint16_t thickness,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x`|左上角 X 坐标|
|`y`|左上角 Y 坐标|
|`width`|矩形宽度|
|`height`|矩形高度|
|`thickness`|边框厚度（像素）|
|`color`|边框颜色|

### `draw_rectangle_by_corners`

指定两个角点坐标绘制矩形边框。

**函数原型**

```c
void draw_rectangle_by_corners(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x0,
	uint16_t y0,
	uint16_t x1,
	uint16_t y1,
	uint16_t thickness,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x0`|左上角 X 坐标|
|`y0`|左上角 Y 坐标|
|`x1`|右下角 X 坐标|
|`y1`|右下角 Y 坐标|
|`thickness`|边框厚度（像素）|
|`color`|边框颜色|

### `fill_rectangle`

指定一点坐标、宽度和高度填充矩形区域。

**函数原型**

```c
void fill_rectangle(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x,
	uint16_t y,
	uint16_t width,
	uint16_t height,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x`|左上角 X 坐标|
|`y`|左上角 Y 坐标|
|`width`|矩形宽度|
|`height`|矩形高度|
|`color`|填充颜色|

### `fill_rectangle_by_corners`

指定两个角点坐标填充矩形区域。

**函数原型**

```c
void fill_rectangle_by_corners(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x0,
	uint16_t y0,
	uint16_t x1,
	uint16_t y1,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x0`|左上角 X 坐标|
|`y0`|左上角 Y 坐标|
|`x1`|右下角 X 坐标|
|`y1`|右下角 Y 坐标|
|`color`|填充颜色|

### `draw_line`

指定两点坐标绘制直线。

**函数原型**

```c
void draw_line(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x0,
	uint16_t y0,
	uint16_t x1,
	uint16_t y1,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x0`|起点 X 坐标|
|`y0`|起点 Y 坐标|
|`x1`|终点 X 坐标|
|`y1`|终点 Y 坐标|
|`color`|线段颜色|

### `draw_circle`

指定圆心坐标和半径绘制圆形。

**函数原型**

```c
void draw_circle(
	uint32_t *buf,
	uint16_t bx,
	int32_t x0,
	int32_t y0,
	int32_t radius,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x0`|圆心 X 坐标|
|`y0`|圆心 Y 坐标|
|`radius`|圆半径|
|`color`|圆形颜色|

### `fill_circle`

指定圆心坐标和半径填充圆形。

**函数原型**

```c
void fill_circle(
	uint32_t *buf,
	uint16_t bx,
	int32_t x0,
	int32_t y0,
	int32_t radius,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x0`|圆心 X 坐标|
|`y0`|圆心 Y 坐标|
|`radius`|圆半径|
|`color`|填充颜色|

### `draw_ellipse`

指定椭圆中心坐标和长短轴绘制椭圆。

**函数原型**

```c
void draw_ellipse(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x0,
	uint16_t y0,
	uint16_t a,
	uint16_t b,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x0`|椭圆中心 X 坐标|
|`y0`|椭圆中心 Y 坐标|
|`a`|椭圆长轴长度|
|`b`|椭圆短轴长度|
|`color`|椭圆颜色|

**说明**
- 规定长轴为 X 轴，短轴为 Y 轴
- 使用中点椭圆算法

### `fill_ellipse`

指定椭圆中心坐标和长短轴填充椭圆。

**函数原型**

```c
void fill_ellipse(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x0,
	uint16_t y0,
	uint16_t a,
	uint16_t b,
	COLOR color
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x0`|椭圆中心 X 坐标|
|`y0`|椭圆中心 Y 坐标|
|`a`|椭圆长轴长度|
|`b`|椭圆短轴长度|
|`color`|填充颜色|

### `bit_blit`

进行位块拷贝操作。

**函数原型**
```c
void bit_blit(
	const uint32_t *src,
	uint16_t src_bx,
	uint16_t src_x,
	uint16_t src_y,
	uint16_t width,
	uint16_t height,
	uint32_t *dst,
	uint16_t dst_bx,
	uint16_t dst_x,
	uint16_t dst_y
);
```

|参数|描述|
|:-:|:-:|
|`src`|源缓冲区指针|
|`src_bx`|源缓冲区宽度（像素）|
|`src_x`|源缓冲区左上角 X 坐标|
|`src_y`|源缓冲区左上角 Y 坐标|
|`width`|拷贝宽度|
|`height`|拷贝高度|
|`dst`|目标缓冲区指针|
|`dst_bx`|目标缓冲区宽度（像素）|
|`dst_x`|目标缓冲区左上角 X 坐标|
|`dst_y`|目标缓冲区左上角 Y 坐标|

### `draw_ascii_string`

使用 PSF 字体绘制 ASCII 字符串。

**函数原型**

```c
void draw_ascii_string(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x,
	uint16_t y,
	COLOR color,
	const char *str,
	const BITMAP_FONT *font
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x`|起始 X 坐标|
|`y`|起始 Y 坐标|
|`color`|字符颜色|
|`str`|字符串内容|
|`font`|使用的 PSF 字体指针|

**说明**

- 仅支持单行 ASCII 字符集

### `draw_unicode_string`

使用 PSF 字体绘制 Unicode 字符串。

**函数原型**

```c
void draw_unicode_string(
	uint32_t *buf,
	uint16_t bx,
	uint16_t x,
	uint16_t y,
	COLOR color,
	const char *str,
	const BITMAP_FONT *font
);
```

|参数|描述|
|:-:|:-:|
|`buf`|绘图缓冲区指针|
|`bx`|绘图缓冲区宽度（像素）|
|`x`|起始 X 坐标|
|`y`|起始 Y 坐标|
|`color`|字符颜色|
|`str`|UTF-8 编码的字符串|
|`font`|使用的 PSF 字体指针|

**说明**

- 支持 UTF-8 编码解码
- 自动处理多字节字符
