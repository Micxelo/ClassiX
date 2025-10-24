# 软盘 - ClassiX 文档

> 当前位置：os/arch/fd.md

## 概述

保护模式中，无法使用 INT 13H 中断，需要通过 8237 DMA 芯片完成软盘读写操作，固定使用 DMA 通道 2。

## 软盘控制器（FDC）

|Port|Read/Write|Register|
|:-:|:-:|:-:|
|`0x3f2`|Write|Digital Output Register|
|`0x3f4`|Read|Floppy Disk Status Register|
|`0x3f5`|Read/Write|Floppy Disk Data Register|
|`0x3f7`|Read|Digital Input Register|
|`0x3f7`|Write|Disk Control Register|

### 数字输出寄存器

|Bit(s)|Name|Description|
|:-:|:-:|:-:|
|0, 1|`DRV_SEL`|Select driver (0-3)|
|2|`RESET`|FDC Reset: 0-reset; 1-normal operation|
|3|`DMA_INT`|DMA interrupt: 0-disable; 1-enable|
|4|`MOT_EN0`|Driver A motor: 0-stop; 1-start|
|5|`MOT_EN1`|Driver B motor: 0-stop; 1-start|
|6|`MOT_EN2`|Driver C motor: 0-stop; 1-start|
|7|`MOT_EN3`|Driver D motor: 0-stop; 1-start|

### 状态寄存器

|Bit|Name|Description|
|:-:|:-:|:-:|
|0|`DAB`|Driver A busy|
|1|`DBB`|Driver B busy|
|2|`DCB`|Driver C busy|
|3|`DDB`|Driver D busy|
|4|`CB`|Controller busy|
|5|`NDM`|DMA set: 0-DMA mode; 1-non-DMA mode|
|6|`DIO`|Direction: 0-CPU->FDD; 1-FDD->CPU|
|7|`RQM`|Data ready: 1-ready for data transfer|

### 数据寄存器

FDC 数据寄存器用于向 FDC 发送控制命令或从 FDC 读取状态。

#### 重新校正（`FD_RECALIBRATE`）

将磁头移动到 track 0，软盘启动时调用。

|Order|D7|D6|D5|D4|D3|D2|D1|D0|Description/Value|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|0|0|0|0|0|0|1|1|1|`0x07`|
|1|0|0|0|0|0|0|US1|US0|Drive number (0-3)|

#### 寻道（`FD_SEEK`）

将磁头移动到指定位置，读/写前调用。

|Order|D7|D6|D5|D4|D3|D2|D1|D0|Description/Value|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|0|0|0|0|0|1|1|1|1|`0x0F`|
|1|0|0|0|0|0|HD|US1|US0|Head number & Drive number|
|2|C|C|C|C|C|C|C|C|Track number (0-79)|

#### 读扇区（`FD_READ`）

|Order|D7|D6|D5|D4|D3|D2|D1|D0|Description/Value|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|0|MT $^{[1]}$|MF $^{[2]}$|SK $^{[3]}$|0|0|1|1|0|`0xE6` (MT=MF=SK=1)|
|1|0|0|0|0|0|0|US1|US0|Drive number|
|2|C|C|C|C|C|C|C|C|Track number (0-79)|
|3|0|0|0|0|0|0|0|HD|Head number (0-1)|
|4|R|R|R|R|R|R|R|R|Start sector number (1-18)|
|5|N|N|N|N|N|N|N|N|Bytes per sector (0=128,1=256,2=512,3=1024)|
|6|EOT|EOT|EOT|EOT|EOT|EOT|EOT|EOT|End of track (last sector number, typically 18)|
|7|GPL|GPL|GPL|GPL|GPL|GPL|GPL|GPL|Sector gap length (typically 0x1B)|
|8|DTL|DTL|DTL|DTL|DTL|DTL|DTL|DTL|Data length (when N=0, typically 0xFF)|

[1] MT：多磁道操作。0-单磁道；1-双磁道。  
[2] MF：记录方式。0-FM（调频）；1-MFM（改进调频）。  
[3] SK：跳过有删除标志的扇区。0-不跳过；1-跳过。

##### 返回值

|Order|Name|Description|
|:-:|:-:|:-:|
|0|`ST0`|Status Register 0|
|1|`ST1`|Status Register 1|
|2|`ST2`|Status Register 2|
|3|C|Current track number|
|4|H|Current head number|
|5|R|Current sector number|
|6|N|Bytes per sector|

`ST0` 的位定义如下：

|Bit(s)|Name|Description|
|:-:|:-:|:-:|
|0, 1|`ST0_DS`|Drive select (0-3)|
|2|`ST0_HA`|Head Address (0-1)|
|3|`ST0_NR`|Not ready (1=drive not ready)|
|4|`ST0_ECE`|Equipment check error (seek failure)|
|5|`ST0_SE`|Seek end (seek completed)|
|6, 7|`ST0_INTR`|Interrupt reason $^{[1]}$|

[1] 00-命令正常结束；01-命令异常结束；10-无效命令；11-驱动器就绪状态改变。

`ST1` 的位定义如下：

|Bit(s)|Name|Description|
|:-:|:-:|:-:|
|0|`ST1_MAM`|Missing address mark|
|1|`ST1_WP`|Write protected|
|2|`ST1_ND`|No data (sector not found)|
|3|`ST1_OR`|Overrun/underrun|
|4|`ST1_CRC`|CRC error|
|5|-|Reserved (0)|
|6|-|Reserved (0)|
|7|`ST1_EOC`|End of cylinder|

`ST2` 的位定义如下：

|Bit(s)|Name|Description|
|:-:|:-:|:-:|
|0|`ST2_MAM`|Missing address mark|
|1|`ST2_BC`|Bad cylinder|
|2|`ST2_SNS`|Scan not satisfied|
|3|`ST2_SEH`|Scan equal hit|
|4|`ST2_WC`|Wrong cylinder (CRC error in ID field)|
|5|`ST2_CRC`|CRC error in data field|
|6|`ST2_CM`|Control mark (deleted data mark)|
|7|-|Reserved (0)|

#### 写扇区（`FD_WRITE`）

|Order|D7|D6|D5|D4|D3|D2|D1|D0|Description/Value|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|0|MT|MF|0|0|0|1|0|1|`0xC5` (MT=MF=1)|
|1|0|0|0|0|0|0|US1|US0|Drive number|
|2|C|C|C|C|C|C|C|C|Track number (0-79)|
|3|0|0|0|0|0|0|0|HD|Head number (0-1)|
|4|R|R|R|R|R|R|R|R|Start sector number (1-18)|
|5|N|N|N|N|N|N|N|N|Bytes per sector (0=128,1=256,2=512,3=1024)|
|6|EOT|EOT|EOT|EOT|EOT|EOT|EOT|EOT|End of track (typically 18)|
|7|GPL|GPL|GPL|GPL|GPL|GPL|GPL|GPL|Sector gap length (typically 0x1B)|
|8|DTL|DTL|DTL|DTL|DTL|DTL|DTL|DTL|Data length (when N=0, typically 0xFF)|

##### 返回值

|Order|Name|Description|
|:-:|:-:|:-:|
|0|`ST0`|Status Register 0|
|1|`ST1`|Status Register 1|
|2|`ST2`|Status Register 2|
|3|C|Current track number|
|4|H|Current head number|
|5|R|Current sector number|
|6|N|Bytes per sector|

#### 检测中断状态（`FD_SENSEI`）

|Order|D7|D6|D5|D4|D3|D2|D1|D0|Description/Value|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|0|0|0|0|0|1|0|0|0|`0x08`|

##### 返回值

|Order|Name|Description|
|:-:|:-:|:-:|
|0|`ST0`|Status Register 0|
|1|PCN|Present Cylinder Number (current track)|

#### 设定驱动器参数（`FD_SPECIFY`）

|Order|D7|D6|D5|D4|D3|D2|D1|D0|Description/Value|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|0|0|0|0|0|0|0|1|1|`0x03`|
|1|SRT $^{[1]}$|SRT|SRT|SRT|HUT $^{[2]}$|HUT|HUT|HUT|Step Rate Time & Head Unload Time|
|2|HLT $^{[3]}$|HLT|HLT|HLT|HLT|HLT|HLT|DMA $^{[4]}$|Head Load Time & DMA mode|

[1] SRT：步进速率时间 `(16 - SRT) * 2ms`。 典型值 0x0F。  
[2] HUT：磁头卸载时间 `(HUT + 1) * 32ms`。 典型值 0x0F。  
[3] HLT：磁头加载时间 `(HLT + 1) * 4ms`。 典型值 0x01。  
[4] DMA：DMA 模式。0-启用 DMA；1-禁用 DMA。 典型值 0x00。

### 数字输入寄存器

`DIR` 仅 D7 位有效，用于表示软盘更换状态。

|Bit|Name|Description|
|:-:|:-:|:-:|
|7|`DSKCHG`|Disk change: 0-disk changed; 1-disk not changed|

### 磁盘控制寄存器

`DCR` 仅 D1、D2 位有效，用于指定数据传输速率。

数据传输速率：

|Rate|Value|Description|
|:-:|:-:|:-:|
|500 kbps|0|High density (1.44MB)|
|300 kbps|1|Double density (360KB)|
|250 kbps|2|Single density (180KB)|
|1 Mbps|3|Extended density (2.88MB)|

## 许可

> 文本内容除另有声明外，均在[署名—非商业性使用—相同方式共享 4.0 协议国际版（CC BY-NC-SA 4.0 International）](https://creativecommons.org/licenses/by-nc-sa/4.0/)下提供。
