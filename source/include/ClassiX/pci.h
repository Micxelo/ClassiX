/*
	include/ClassiX/pci.h
*/

#ifndef _CLASSIX_PCI_H_
#define _CLASSIX_PCI_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

/* PCI 设备类型 */
typedef enum {
	PCI_DEV_UNKNOWN,	/* 未知设备 */
	PCI_DEV_SCSI,		/* SCSI 控制器 */
	PCI_DEV_IDE,		/* IDE 控制器 */
	PCI_DEV_FDC,		/* 软盘控制器 */
	PCI_DEV_ATA,		/* ATA 控制器 */
	PCI_DEV_SATA,		/* SATA 控制器 */
	PCI_DEV_SAS,		/* SAS 控制器 */
	PCI_DEV_NVME,		/* NVMe 控制器 */
	PCI_DEV_NIC,		/* 网络控制器 */
	PCI_DEV_VGA,		/* VGA 兼容控制器 */
	PCI_DEV_BRIDGE,		/* 桥设备 */
	PCI_DEV_USB			/* USB 控制器 */
} PCI_DEVICE_TYPE;

/* PCI 设备信息 */
typedef struct {
	uint8_t bus;
	uint8_t device;
	uint8_t function;

	uint16_t vendor_id;
	uint16_t device_id;

	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
	uint8_t revision;

	PCI_DEVICE_TYPE type;

	uint32_t bars[6];	/* BAR0-BAR5 */
	uint8_t interrupt_pin;
	uint8_t interrupt_line;
} PCI_DEVICE;

/* PCI 设备列表 */
typedef struct {
	PCI_DEVICE* devices;
	int32_t count;
} PCI_DEVICE_LIST;

extern PCI_DEVICE_LIST pci_devices;

/* 物理区域描述符 */
typedef struct __attribute__((packed)) {
	uint32_t phys;		/* 物理地址 */
	uint16_t length;	/* 传输长度 */
	uint16_t eot;		/* 结束标志 (0x8000 表示最后一个描述符) */
} PRD_ENTRY;

void pci_enable_device(uint8_t bus, uint8_t device, uint8_t function);
uint32_t pci_get_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index);

void pci_scan_devices(void);
PCI_DEVICE* pci_find_device(PCI_DEVICE_TYPE type);
void pci_free_device_list(void);

#ifdef __cplusplus
	}
#endif

#endif