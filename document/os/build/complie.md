# 编译 - ClassiX 文档

> 当前位置: os/build/complie.md

## 系统要求

ClassiX 在如下环境中编译通过：

- Ubuntu 24.04.3 LTS (Windows Subsystem for Linux)
- GCC version 13.3.0
- NASM version 2.16.01
- LD version 2.42

## 安装工具链

```shell
sudo apt install build-essential nasm
```

## 克隆仓库

```shell
git clone https://github.com/Micxelo/ClassiX.git
cd ClassiX/source
```

## 构建系统

```shell
make
```

## 清理

```shell
make clean
```

## 编译选项

### CFLAGS

|选项|含义|
|:-:|:-|
|`-O2`|启用中等优化级别|
|`-m32`|生成 32 位架构的目标代码|
|`-std=gnu99`|使用 C99 标准的 GNU 扩展语法|
|`-Wall`|启用大部分警告信息|
|`-Wextra`|启用额外警告|
|`-Wno-unused-parameter`|忽略未使用的函数参数警告|
|`-Wno-unused-function`|忽略未使用的静态函数警告|
|`-Werror=parentheses`|将括号相关的警告视为错误|
|`-ffreestanding`|编译独立环境程序|
|`-fleading-underscore`|在符号名前自动添加 `_` 前缀|
|`-fno-pic`|禁用位置无关代码|
|`-fno-stack-protector`|禁用栈溢出保护|
|`-nostartfiles`|不链接标准启动文件|
|`-nostdinc`|不搜索标准头文件目录|

### ASFLAGS

|选项|含义|
|:-:|:-|
|`-f elf32`|指定输出目标文件格式为 32 位 ELF 格式|

### LDFLAGS

|选项|含义|
|:-:|:-|
|`-T $(LDSCRIPT)`|使用自定义的链接脚本|
|`-melf_i386`|使用 i386 架构的 ELF 可执行文件|
|`-nostdlib`|禁用所有标准库|
|`-z noexecstack`|强制标记堆栈空间不可执行|
