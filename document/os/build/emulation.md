# 使用虚拟机运行 - ClassiX 文档

> 当前位置: os/build/emulation.md

## 获取硬盘镜像

### 构建镜像文件

#### 创建镜像文件

```shell
dd if=/dev/zero of=hd.img bs=1M count=32
```

#### 分区

建议分区方案：

|分区号|类型|大小|用途|
|:-:|:-:|:-:|:-:|
|1|主分区|8 MiB|启动分区|
|2|主分区|剩余空间|根分区|

> **注意**  
> 必须使用 MBR 分区表，否则系统无法正确识别分区。

```shell
parted hd.img --script -- mklabel msdos
parted hd.img --script -- mkpart primary 1MiB 9MiB
parted hd.img --script -- mkpart primary 9MiB 100%
```

#### 格式化分区

> **注意**  
> 启动分区必须格式化为 ext2 文件系统，否则无法引导。  
> 根分区仅支持 FAT12/16 文件系统。

```shell
sudo losetup -Pf hd.img
sudo mkfs.ext2 /dev/loop0p1
sudo mkfs.vfat -F 16 /dev/loop0p2
```

> **注意**  
> 请根据实际情况调整 `/dev/loop0`，可使用 `losetup -f` 获取下一个可用的 loop 设备。

#### 安装 GRUB

```shell
sudo mount /dev/loop0p1 /mnt
sudo grub-install --target=i386-pc --boot-directory=/mnt/boot /dev/loop0
sudo mkdir -p /mnt/boot/grub
sudo vim /mnt/boot/grub/grub.cfg
```

`grub.cfg` 内容示例：

```
set timeout=-1
set default=0

insmod vbe
insmod gfxterm
insmod videotest

menuentry "ClassiX" {
	terminal_output gfxterm
	multiboot /boot/ClassiX.sys
	boot
}
```

#### 复制文件

```shell
sudo cp /path/to/ClassiX.sys /mnt/boot/
```

#### 卸载镜像

```shell
sudo umount /mnt
sudo losetup -d /dev/loop0
```

### 下载预构建镜像

## 使用虚拟机运行

### 安装 QEMU

```shell
sudo apt install qemu-system
```

### 运行虚拟机

```shell
qemu-system-i386 -m 128M -vga std -serial stdio -nic none -drive if=ide,file=/path/to/hd.img,format=raw -drive if=floppy,file=/path/to/fd.img,format=raw
```

|选项|含义|
|:-:|:-|
|`-m 128M`|分配 128 MiB 内存|
|`-vga std`|使用标准 VGA 显卡|
|`-serial stdio`|将串口输出重定向到标准输入输出|
|`-nic none`|禁用网络接口|
|`-drive`|使用指定的镜像文件|
