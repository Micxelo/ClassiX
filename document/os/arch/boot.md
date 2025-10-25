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

---

上一节：[编译](./complie.md)
