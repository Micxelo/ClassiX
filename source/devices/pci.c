/*
	devices/pci.c
*/

#include <ClassiX/debug.h>
#include <ClassiX/memory.h>
#include <ClassiX/io.h>
#include <ClassiX/pci.h>
#include <ClassiX/typedef.h>

/* PCI 配置空间访问端口 */
#define PCI_CONFIG_ADDRESS					0x0CF8
#define PCI_CONFIG_DATA						0x0CFC

/* PCI 配置空间偏移量 */
#define PCI_VENDOR_ID						0x00
#define PCI_DEVICE_ID						0x02
#define PCI_COMMAND							0x04
#define PCI_STATUS							0x06
#define PCI_REVISION_ID						0x08
#define PCI_PROG_IF							0x09
#define PCI_SUBCLASS						0x0A
#define PCI_CLASS_CODE						0x0B
#define PCI_CACHE_LINE_SIZE					0x0C
#define PCI_LATENCY_TIMER					0x0D
#define PCI_HEADER_TYPE						0x0E
#define PCI_BIST							0x0F
#define PCI_BAR0							0x10
#define PCI_BAR1							0x14
#define PCI_BAR2							0x18
#define PCI_BAR3							0x1C
#define PCI_BAR4							0x20
#define PCI_BAR5							0x24
#define PCI_INTERRUPT_LINE					0x3C
#define PCI_INTERRUPT_PIN					0x3D

/*  PCI 头类型 */
#define PCI_HEADER_TYPE_DEVICE				0
#define PCI_HEADER_TYPE_BRIDGE				1
#define PCI_HEADER_TYPE_CARDBUS				2

/* PCI 命令寄存器位 */
#define PCI_CMD_IO_ENABLE					(1 << 0)
#define PCI_CMD_MEM_ENABLE					(1 << 1)
#define PCI_CMD_BUS_MASTER					(1 << 2)
#define PCI_CMD_INTX_DISABLE				(1 << 10)

PCI_DEVICE_LIST pci_devices;

/*
	@brief 生成 PCI 配置空间地址。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@param offset 寄存器偏移量
	@return 配置空间地址
*/
static inline uint32_t pci_make_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	return (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
}

/*
	@brief 按双字读取 PCI 配置空间。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@param offset 寄存器偏移量
	@return 读取的双字值
*/
static uint32_t pci_read_config32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t address = pci_make_address(bus, device, function, offset);
	out32(PCI_CONFIG_ADDRESS, address);
	return in32(PCI_CONFIG_DATA);
}

/*
	@brief 按双字写入 PCI 配置空间。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@param offset 寄存器偏移量
	@param value 写入的双字值
*/
static void pci_write_config32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
	uint32_t address = pci_make_address(bus, device, function, offset);
	out32(PCI_CONFIG_ADDRESS, address);
	out32(PCI_CONFIG_DATA, value);
}

/*
	@brief 按字读取 PCI 配置空间。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@param offset 寄存器偏移量
	@return 读取的字值
*/
static uint16_t pci_read_config16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t value = pci_read_config32(bus, device, function, offset);
	return (uint16_t) (value >> ((offset & 2) * 8));
}

/*
	@brief 按字写入 PCI 配置空间。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@param offset 寄存器偏移量
	@param value 写入的字值
*/
static void pci_write_config16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value)
{
	uint32_t current = pci_read_config32(bus, device, function, offset & ~3);
	uint32_t shift = (offset & 2) * 8;
	uint32_t mask = 0xFFFF << shift;
	uint32_t new_value = (current & ~mask) | ((value << shift) & mask);
	pci_write_config32(bus, device, function, offset & ~3, new_value);
}

/*
	@brief 按字节读取 PCI 配置空间。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@param offset 寄存器偏移量
	@return 读取的字节值
*/
static uint8_t pci_read_config8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
	uint32_t value = pci_read_config32(bus, device, function, offset);
	return (uint8_t) (value >> ((offset & 3) * 8));
}

/*
	@brief 按字节写入 PCI 配置空间。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@param offset 寄存器偏移量
	@param value 写入的字节值
*/
static void pci_write_config8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value)
{
	uint32_t current = pci_read_config32(bus, device, function, offset & ~3);
	uint32_t shift = (offset & 3) * 8;
	uint32_t mask = 0xFF << shift;
	uint32_t new_value = (current & ~mask) | ((value << shift) & mask);
	pci_write_config32(bus, device, function, offset & ~3, new_value);
}

/*
	@brief 获取 PCI 头类型。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@return 头类型值
*/
static uint8_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t function)
{
	return pci_read_config8(bus, device, function, PCI_HEADER_TYPE);
}

/*
	@brief 根据类代码和子类代码获取 PCI 设备类型。
	@param class_code 设备类代码
	@param subclass 设备子类代码
	@return PCI 设备类型枚举值
*/
static PCI_DEVICE_TYPE pci_get_device_type(uint8_t class_code, uint8_t subclass)
{
	switch (class_code) {
		case 0x01: /* 大容量存储控制器 */
			switch (subclass) {
				case 0x00: /* SCSI 控制器 */
					return PCI_DEV_SCSI;
				case 0x01: /* IDE 控制器 */
					return PCI_DEV_IDE;
				case 0x02: /* 软盘控制器 */
					return PCI_DEV_FDC;
				case 0x05: /* ATA 控制器 */
					return PCI_DEV_ATA;
				case 0x06: /* SATA 控制器 */
					return PCI_DEV_SATA;
				case 0x07: /* SAS 控制器 */
					return PCI_DEV_SAS;
				case 0x08: /* NVMe 控制器 */
					return PCI_DEV_NVME;
				default:
					return PCI_DEV_UNKNOWN;
			}
		case 0x02: /* 网络控制器 */
			return PCI_DEV_NIC;
		case 0x03: /* 显示控制器 */
			switch (subclass) {
				case 0x00: /* VGA 兼容控制器 */
					return PCI_DEV_VGA;
				default:
					return PCI_DEV_UNKNOWN;
			}
		case 0x06: /* 桥设备 */
			switch (subclass) {
				case 0x00: /* 主机桥 */
				case 0x01: /* ISA 桥 */
				case 0x04: /* PCI-PCI 桥 */
				case 0x06: /* 透明桥 */
					return PCI_DEV_BRIDGE;
				default:
					return PCI_DEV_UNKNOWN;
			}
		case 0x0C: /* 串行总线控制器 */
			switch (subclass) {
				case 0x03: /* USB 控制器 */
					return PCI_DEV_USB;
				default:
					return PCI_DEV_UNKNOWN;
			}
		default:
			return PCI_DEV_UNKNOWN;
	}
}

/*
	@brief 检查指定 PCI 设备是否存在。
	@param bus 总线号
	@param device 设备号
	@param function 功能号
	@return 存在返回 true，否则返回 false
*/
static bool pci_device_exists(uint8_t bus, uint8_t device, uint8_t function)
{
	uint16_t vendor_id = pci_read_config16(bus, device, function, PCI_VENDOR_ID);
	return vendor_id != 0xFFFF;
}

/*
	@brief 获取指定 PCI 设备的类代码和子类代码。
	@param bus PCI 总线号
	@param device PCI 设备号
	@param function PCI 功能号
	@param class_code 返回的类代码指针
	@param subclass 返回的子类代码指针
*/
static void pci_get_class_codes(uint8_t bus, uint8_t device, uint8_t function, uint8_t *class_code, uint8_t *subclass)
{
	uint32_t reg = pci_read_config32(bus, device, function, PCI_CLASS_CODE);
	*class_code = reg >> 24;
	*subclass = reg >> 16;
}

/*
	@brief 启用指定 PCI 设备。
	@param bus PCI 总线号
	@param device PCI 设备号
	@param function PCI 功能号
*/
void pci_enable_device(uint8_t bus, uint8_t device, uint8_t function)
{
	uint16_t command = pci_read_config16(bus, device, function, PCI_COMMAND);

	/* 启用内存空间、I/O空间和总线主控 */
	command |= PCI_CMD_MEM_ENABLE | PCI_CMD_IO_ENABLE | PCI_CMD_BUS_MASTER;

	pci_write_config16(bus, device, function, PCI_COMMAND, command);
}

/*
	@brief 获取指定 PCI 设备的 BAR（基址寄存器）地址。
	@param bus PCI 总线号
	@param device PCI 设备号
	@param function PCI 功能号
	@param bar_index BAR 索引（0~5）
	@return BAR 地址（I/O 或内存地址）
*/
uint32_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index)
{
	if (bar_index > 5)
		return 0;

	uint32_t bar = pci_read_config32(bus, device, function, PCI_BAR0 + bar_index * 4);

	if (bar & 1) /* I/O 映射，返回端口号 */
		return bar & ~3;
	else /* 内存映射，返回地址 */
		return bar & ~0xF;
}

/*
	@brief 扫描所有 PCI 设备并保存到设备列表。
*/
void pci_scan_devices(void)
{
	pci_devices.count = 0;
	pci_devices.devices = (PCI_DEVICE*) kmalloc(sizeof(PCI_DEVICE) * 256);

	for (int32_t bus = 0; bus < 256; bus++) {
		for (int32_t device = 0; device < 32; device++) {
			/* 检查设备是否存在 (function 0) */
			uint32_t vendor_device = pci_read_config32(bus, device, 0, PCI_VENDOR_ID);
			uint16_t vendor_id = vendor_device;

			if (vendor_id == 0xFFFF)
				continue; /* 设备不存在 */

			/* 获取头类型 */
			uint8_t header_type = pci_get_header_type(bus, device, 0);
			uint8_t functions = (header_type & 0x80) ? 8 : 1; /* 多功能设备 */

			for (uint8_t function = 0; function < functions; function++) {
				/* 检查当前功能是否存在 */
				vendor_device = pci_read_config32(bus, device, function, PCI_VENDOR_ID);
				vendor_id = vendor_device;
				if (vendor_id == 0xFFFF)
					continue; /* 功能不存在 */

				/* 获取设备信息 */
				PCI_DEVICE *dev = &pci_devices.devices[pci_devices.count++];
				dev->bus = bus;
				dev->device = device;
				dev->function = function;
				dev->vendor_id = vendor_id;
				dev->device_id = vendor_device >> 16;

				/* 获取类代码和子类 */
				uint32_t class_reg = pci_read_config32(bus, device, function, PCI_CLASS_CODE);
				dev->class_code = class_reg >> 24;
				dev->subclass = class_reg >> 16;
				dev->prog_if = class_reg >> 8;
				dev->revision = class_reg;

				/* 确定设备类型 */
				dev->type = pci_get_device_type(dev->class_code, dev->subclass);

				/* 获取 BAR 寄存器 */
				dev->bars[0] = pci_read_config32(bus, device, function, PCI_BAR0);
				dev->bars[1] = pci_read_config32(bus, device, function, PCI_BAR1);
				dev->bars[2] = pci_read_config32(bus, device, function, PCI_BAR2);
				dev->bars[3] = pci_read_config32(bus, device, function, PCI_BAR3);
				dev->bars[4] = pci_read_config32(bus, device, function, PCI_BAR4);
				dev->bars[5] = pci_read_config32(bus, device, function, PCI_BAR5);

				/* 获取中断信息 */
				uint32_t interrupt_reg = pci_read_config32(bus, device, function, PCI_INTERRUPT_LINE);
				dev->interrupt_pin = interrupt_reg >> 8;
				dev->interrupt_line = interrupt_reg ;

				debug("PCI: PCI %02x:%02x.%x: Vendor=%04x Device=%04x Class=%02x Subclass=%02x Type=%d\n",
						bus, device, function, vendor_id, dev->device_id,
						dev->class_code, dev->subclass, dev->type);
			}
		}
	}
}

/*
	@brief 查找指定类型的 PCI 设备。
	@param type 设备类型（PCI_DEVICE_TYPE 枚举）
	@return 匹配的 PCI_DEVICE 指针，未找到返回 NULL
*/
PCI_DEVICE *pci_find_device(PCI_DEVICE_TYPE type)
{
	for (int32_t i = 0; i < pci_devices.count; i++)
		if (pci_devices.devices[i].type == type)
			return &pci_devices.devices[i];

	return NULL;
}

/*
	@brief 释放 PCI 设备列表所占内存。
*/
void pci_free_device_list(void)
{
	if (pci_devices.devices)
		kfree(pci_devices.devices);
}
