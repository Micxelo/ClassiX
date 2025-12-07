# ClassiX

ClassiX 是一个从零开始编写的 32 位操作系统内核，支持基础硬件驱动、图形用户界面和多任务处理。

![ClassiX](https://img.shields.io/badge/Kernel-ClassiX-blue?style=flat-square)
[![License: GPL](https://img.shields.io/badge/License-GPL-yellow?style=flat-square)](https://www.gnu.org/licenses/gpl-3.0.txt)
[![Wiki](https://img.shields.io/badge/Wiki-DeepWiki-cyan?style=flat-square)](https://deepwiki.com/Micxelo/ClassiX)

## 特性概览

- 32 位保护模式运行
- 物理内存管理
- PS/2 键盘、鼠标驱动
- VBE 图形模式显示支持
- 块设备驱动
- 多任务处理支持

## 构建

```shell
# 安装工具链
sudo apt install build-essential nasm

# 克隆仓库
git clone https://github.com/Micxelo/ClassiX.git
cd ClassiX/source

# 构建系统
make
```

## 参与贡献

欢迎通过 Issue 和 PR 参与项目开发。

1. 提交 Issue 报告问题或建议新功能
2. Fork 仓库并创建特性分支
3. 提交清晰的 commit 消息
4. 创建 Pull Request 并描述变更内容

## 许可

除非另有声明，本项目的源代码均在 [GNU 通用公共许可证（GNU GPL v3）](https://www.gnu.org/licenses/gpl-3.0.txt)下发布。
