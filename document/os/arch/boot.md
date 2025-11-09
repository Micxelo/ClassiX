# 启动 - ClassiX 文档

> 当前位置：os/arch/boot.md

## 引导

ClassiX 被设计为使用 Multiboot 1 规范引导方式。尽管兼容 Multiboot 规范的引导加载程序都可以引导 ClassiX，但最常见的选择可能是 GRUB。

## Multiboot 规范概述

Multiboot 规范是一个由 GNU 项目制定的标准，旨在为不同的操作系统提供统一的引导接口。遵循该规范的操作系统可以被任何兼容 Multiboot 的引导加载程序加载，从而提高了操作系统的可移植性和兼容性。

对于 ClassiX 而言，遵循 Multiboot 规范意味着：

- 内核镜像包含特定的 Multiboot 头部，供引导加载程序识别
- 引导加载程序会为内核提供标准化的系统信息和环境
- 内核可以依赖引导加载程序完成一些基本的硬件初始化

## 内存布局

### 物理地址空间

内核通过链接器脚本 `linker.ld` 定义内存布局：

|地址|对齐|段|描述|
|:-:|:-:|:-:|:-:|
|`0x00100000`|1K|`.classix`|ClassiX 头|
|`0x00100400`|1K|`.multiboot`|Multiboot 头|
|`0x00100800`|1K|`.text`|代码段|
|可变|1K|`.rodata`|只读数据段|
|可变|1K|`.data`|已初始化数据段|
|可变|4K|`.bss`|未初始化数据段|

### 链接器符号

|符号|描述|
|:-:|:-:|
|`load_phys`|加载段起始地址|
|`load_end_phys`|加载段结束地址|
|`bss_start_phys`|BSS 段起始地址|
|`bss_end_phys`|BSS 段结束地址|
|`kernel_start_phys`|内核物理起始地址|
|`kernel_end_phys`|内核物理结束地址|

## ClassiX 头部结构

## Multiboot 头部结构

|偏移|字段|值|描述|
|:-:|:-:|:-:|:-:|
|0|magic|`0x1BADB002`|Multiboot 魔数|
|4|flags|`0x00010007`|特征标志位|
|8|checksum|`-(0x1BADB002 + 0x00010007)`|校验和|
|12|header_addr|`multiboot_header_phys`|头结构物理地址|
|16|load_addr|`load_phys`|加载段起始地址|
|20|load_end_addr|`load_end_phys`|加载段结束地址|
|24|bss_end_addr|`bss_end_phys`|BSS 段结束地址|
|28|entry_addr|`entry_phys`|入口点地址|

### 标志位定义

|标志位|值|描述|
|:-:|:-:|:-:|
|`MBALIGN`|`1 << 0`|要求模块按页对齐|
|`MEMINFO`|`1 << 1`|要求提供内存信息|
|`VIDEOMODE`|`1 << 2`|要求设置视频模式|
|`ADDRWORD`|`1 << 16`|使用地址字段|
