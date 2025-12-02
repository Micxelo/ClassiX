/*
	core/services/service.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/debug.h>
#include <ClassiX/fatfs.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/memory.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>

#include <string.h>

#define SERVICE_HEADER_SIGNATURE			(0x43565348)	/* "CVSH" */

typedef struct __attribute__((packed)) {
	/* 基础信息区 */
	uint32_t signature;			/* 签名 */
	uint32_t version;			/* 版本号 */
	uint32_t flags;				/* 标志位 */

	/* 内存布局 */
	uint32_t entry_point;		/* 程序入口点 */
	uint32_t code_size;			/* 代码段大小 */
	uint32_t data_size;			/* 数据段大小 */
	uint32_t bss_size;			/* BSS 段大小 */
	uint32_t heap_size;			/* 堆大小 */

	/* 文件布局 */
	uint32_t code_offset;		/* 代码段文件偏移 */
	uint32_t data_offset;		/* 数据段文件偏移 */
	uint32_t stack_size;		/* 栈大小 */
	uint32_t total_size;		/* 总内存需求 */

	/* 保留字段 */
	uint32_t reserved0;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
} SERVICE_HEADER;

extern void service_start(uint32_t eip, uint32_t cs, uint32_t esp, uint32_t ds, uint32_t *tss_esp0);

int32_t app_start(FAT_FILE *file, const char *cmdline)
{
	/* 参数检查 */
	if (NULL == file) {
		debug("SERVICE: Invalid file pointer in app_start.\n");
		return -1;
	}

	TASK *task = task_get_current();
	
	uint8_t *buf = kmalloc(file->entry->file_size);
	if (NULL == buf) {
		debug("SERVICE: Failed to allocate memory for service file.\n");
		return -2; /* 分配内存失败 */
	}

	uint32_t bytes_read = 0;
	fatfs_read_file(file, buf, file->entry->file_size, &bytes_read);
	if (bytes_read != file->entry->file_size) {
		kfree(buf);
		debug("SERVICE: Failed to read complete service file.\n");
		return -3; /* 读取文件失败 */
	}

	SERVICE_HEADER *header = (SERVICE_HEADER *) buf;
	if (header->signature != SERVICE_HEADER_SIGNATURE) {
		kfree(buf);
		debug("SERVICE: Invalid service file signature.\n");
		return -4; /* 文件格式错误 */
	}

	uint8_t *mem = kmalloc(header->total_size);
	if (NULL == mem) {
		kfree(buf);
		debug("SERVICE: Failed to allocate memory for service execution.\n");
		return -5; /* 分配内存失败 */
	}

	/* 清零全部内存 */
	memset(mem, 0, header->total_size);

	/* 加载各段 */
	memcpy(mem, buf + header->code_offset, header->code_size);						/* 代码段 */
	memcpy(mem + header->code_size, buf + header->data_offset, header->data_size);	/* 数据段 */
	memset(mem + header->code_size + header->data_size, 0, header->bss_size);		/* BSS段清零 */

    /* 计算栈指针位置（栈位于堆之后） */
	uint32_t app_esp = header->code_size + header->data_size + header->bss_size + header->heap_size + header->stack_size;

	task->ds_base = (uint32_t) mem;

	/* 设置LDT描述符：代码段和数据段 */
	/* 代码段描述符 */
	gdt_set_gate(task->selector + 0, (uint32_t) mem, header->code_size - 1, AR_3_CODE32_ER & 0xff, AR_3_CODE32_ER >> 8);
	/* 数据段描述符（包含BSS、堆和栈） */
	gdt_set_gate(task->selector + 1, (uint32_t) mem + header->code_size,
		header->data_size + header->bss_size + header->heap_size + header->stack_size - 1,
		AR_3_DATA32_RW & 0xff, AR_3_DATA32_RW >> 8);

	/* 复制数据段初始值（如果header中有数据段偏移） */
	if (header->data_size > 0 && header->data_offset > 0)
		memcpy(mem + header->code_size, buf + header->data_offset, header->data_size);

    service_start((uint32_t) mem + header->entry_point, 0 * 8 + 4, app_esp, 1 * 8 + 4, &task->tss.esp0);

	/* 释放临时缓冲区 */
	kfree(buf);
	return 0; /* 成功 */	
}
