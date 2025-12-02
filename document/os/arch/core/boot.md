# 启动 - ClassiX 文档

> 当前位置: os/arch/core/boot.md

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
|`0x00100000`|1K|`.classix`|ClassiX 签名|
|`0x00100400`|1K|`.multiboot`|Multiboot 签名|
|`0x00100800`|1K|`.text`|代码段|
|-|1K|`.rodata`|只读数据段|
|-|1K|`.data`|已初始化数据段|
|-|4K|`.bss`|未初始化数据段|

## 链接器符号

|符号|描述|
|:-:|:-:|
|`load_phys`|加载段起始地址|
|`load_end_phys`|加载段结束地址|
|`bss_start_phys`|BSS 段起始地址|
|`bss_end_phys`|BSS 段结束地址|
|`kernel_start_phys`|内核物理起始地址|
|`kernel_end_phys`|内核物理结束地址|

## ClassiX 签名

|偏移|字段|值|描述|
|:-:|:-:|:-:|:-:|
|0|magic|`0xCD59F115`|ClassiX 魔数|
|4|version|-|内核版本|
|8|entry|-|入口点|
|12|size|-|内核大小|

## Multiboot 签名

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

## 启动流程

### 引导加载程序
- GRUB 读取磁盘中的内核镜像文件
- 在文件前 8KiB 范围内搜索 Multiboot 头部签名 `0x1BADB002` 并验证校验和

### 内核加载
- GRUB 根据 Multiboot 头部的地址字段将内核加载到指定物理地址
- 加载段被复制到 `0x00100800` 开始的物理内存

### 环境准备
- GRUB 切换处理器到 32 位保护模式
- 禁用硬件中断和分页机制
- 设置 EBX 指向 Multiboot 信息结构，EAX 包含 Multiboot 魔数

### 内核初始化
- 跳转到 `start` 标签处开始执行
- 设置内核栈空间
- 重置 EFLAGS 寄存器状态
- 调用 C 语言入口函数 `_main`
- 清零 BSS 段
