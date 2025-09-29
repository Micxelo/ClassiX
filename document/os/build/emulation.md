# 使用虚拟机运行 - ClassiX 文档

> 当前位置：os/build/emulation.md

## 获取硬盘镜像

### 构建镜像文件

使用 `dd` 命令创建一个 32 MiB 的文件：

```shell
dd if=/dev/zero of=disk.img bs=1M count=32
```

## 许可

> 文本内容除另有声明外，均在[署名—非商业性使用—相同方式共享 4.0 协议国际版（CC BY-NC-SA 4.0 International）](https://creativecommons.org/licenses/by-nc-sa/4.0/)下提供。

---

上一节：[编译](./complie.md)
