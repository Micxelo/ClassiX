/*
	core/services/service.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/crc.h>
#include <ClassiX/debug.h>
#include <ClassiX/fatfs.h>
#include <ClassiX/interrupt.h>
#include <ClassiX/memory.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>

#include <string.h>

#define SERVICE_HEADER_SIGNATURE_CONSOLE	(0x43565253)	/* "SRVC" */
#define SERVICE_HEADER_SIGNATURE_WINDOW		(0x57565253)	/* "SRIW" */

typedef struct __attribute__((packed)) {
	uint32_t signature;		/* 文件签名 */
	union {
		uint32_t version;	/* 版本号 */
		struct {
			uint16_t patch;	/* 修订号 */
			uint8_t minor;	/* 次版本号 */
			uint8_t major;	/* 主版本号 */
		};
	};
	uint32_t crc32;			/* CRC32 校验值 */
	uint32_t total_size;	/* 文件总大小 */

	uint32_t entry_point;	/* 入口点偏移 */
	uint32_t code_offset;	/* 代码段偏移 */
	uint32_t code_size;		/* 代码段大小 */
	uint32_t rodata_offset;	/* 只读数据段偏移 */
	uint32_t rodata_size;	/* 只读数据段大小 */
	uint32_t data_offset;	/* 数据段文件偏移 */
	uint32_t data_size;		/* 数据段大小 */
	uint32_t bss_size;		/* BSS段大小 */
	uint32_t stack_size;	/* 栈大小 */
	uint32_t heap_size;		/* 堆大小 */
	uint32_t alignment;		/* 内存对齐要求 */

	uint32_t reserved;		/* 保留字段 */
} SERVICE_HEADER;

extern void service_start(uint32_t eip, uint32_t cs, uint32_t esp, uint32_t ds, uint32_t *tss_esp0);

/*
	@brief 设置 LDT 描述符。
	@param desc 描述符指针
	@param base 段基址
	@param limit 段界限
	@param ar 访问权限
*/
static void set_ldt_descriptor(SEGMENT_DESCRIPTOR *desc, uint32_t base, uint32_t limit, uint32_t ar)
{
	if (limit > 0xFFFFF) {
		ar |= 0x0800;
		limit >>= 12;
	}

	desc->limit_low = limit & 0xFFFF;
	desc->base_low = base & 0xFFFF;
	desc->base_mid = (base >> 16) & 0xFF;
	desc->access = ar & 0xFF;

	desc->granularity = ((limit >> 16) & 0x0F) | (((ar >> 8) & 0x0F) << 4);
	desc->base_high  = (base >> 24) & 0xFF;
}

int32_t app_start(FAT_FILE *file, const char *cmdline)
{
	uint8_t *buf = NULL;
	uint8_t *mem = NULL;
	int32_t result = 0;
	TASK *task = task_get_current();

	/* 参数检查 */
	if (NULL == file) {
		debug("SERVICE: Invalid file pointer in app_start.\n");
		return -1;
	}

	buf = kmalloc(file->entry->file_size);
	if (NULL == buf) {
		debug("SERVICE: Failed to allocate memory for service file.\n");
		return -2; /* 分配内存失败 */
	}

	uint32_t bytes_read = 0;
	fatfs_read_file(file, buf, file->entry->file_size, &bytes_read);
	if (bytes_read != file->entry->file_size) {
		debug("SERVICE: Failed to read complete service file.\n");
		result = -3; /* 读取文件失败 */
		goto clean;
	}

	SERVICE_HEADER *header = (SERVICE_HEADER *) buf;
	if (header->signature != SERVICE_HEADER_SIGNATURE_CONSOLE && header->signature != SERVICE_HEADER_SIGNATURE_WINDOW) {
		debug("SERVICE: Invalid service file signature.\n");
		result = -4; /* 文件格式错误 */
		goto clean;
	}

	if (header->total_size > file->entry->file_size) {
		debug("SERVICE: Service file size mismatch.\n");
		result = -4; /* 文件格式错误 */
		goto clean;
	}

	/* 计算 CRC32 校验值并验证 */
	uint32_t crc = header->crc32;
	header->crc32 = 0; /* 计算 CRC 时将该字段置零 */
	if (crc != crc32(buf, header->total_size)) {
		debug("SERVICE: CRC32 checksum mismatch.\n");
		result = -4; /* 文件格式错误 */
		goto clean;
	}

	/* 计算代码段和数据段大小，考虑对齐 */
	uint32_t code_segment_size = ALIGN_UP(header->code_size, header->alignment);
	uint32_t data_segment_size =
		ALIGN_UP(header->rodata_size, header->alignment) +
		ALIGN_UP(header->data_size, header->alignment) +
		header->bss_size +
		header->heap_size +
		header->stack_size;

	/* 确保数据段大小不为零，以便正确设置段界限 */
	if (data_segment_size == 0) data_segment_size = 1;

	if (code_segment_size == 0) {
		debug("SERVICE: Service file has no code segment.\n");
		result = -4; /* 文件格式错误 */
		goto clean;
	}

	if (header->entry_point >= code_segment_size) {
		debug("SERVICE: Entry point offset is out of code segment bounds.\n");
		result = -4; /* 文件格式错误 */
		goto clean;
	}

	mem = kmalloc(code_segment_size + data_segment_size);
	if (NULL == mem) {
		debug("SERVICE: Failed to allocate memory for service execution.\n");
		result = -2; /* 分配内存失败 */
		goto clean;
	}

	debug("SERVICE: Loaded service file\n");
	debug("  Version: %u.%u.%u\n", header->major, header->minor, header->patch);
	debug("  Entry point offset: 0x%x\n", header->entry_point);
	debug("  Code segment: offset = 0x%x, size = %u bytes\n", header->code_offset, header->code_size);
	debug("  Rodata segment: offset = 0x%x, size = %u bytes\n", header->rodata_offset, header->rodata_size);
	debug("  Data segment: offset = 0x%x, size = %u bytes\n", header->data_offset, header->data_size);
	debug("  BSS segment size: %u bytes\n", header->bss_size);
	debug("  Stack size: %u bytes\n", header->stack_size);
	debug("  Heap size: %u bytes\n", header->heap_size);
	debug("  Alignment: %u bytes\n", header->alignment);

	/* 加载各段 */
	memcpy(mem, buf + header->code_offset, code_segment_size + data_segment_size - header->bss_size - header->heap_size - header->stack_size);

	/* 设置任务的段基址和界限 */
	task->code_base = (uint32_t) mem;
	task->code_limit = code_segment_size - 1;
	task->data_base = (uint32_t) mem + ALIGN_UP(header->code_size, header->alignment);
	task->data_limit = data_segment_size - 1;

	/* BSS 段、堆和栈清零 */
	memset(mem + code_segment_size + header->data_size, 0, header->bss_size + header->heap_size + header->stack_size);

	/* 设置 LDT 描述符 */
	set_ldt_descriptor(&task->ldt[0], task->code_base, task->code_limit, AR_3_CODE32_ER);
	set_ldt_descriptor(&task->ldt[1], task->data_base, task->data_limit, AR_3_DATA32_RW);

	/* 设置 LDT 选择子 */
	task->tss.ldtr = task->selector + (MAX_TASKS * 8);

	/* 段选择子（LDT 索引 0 和 1，TI=1，RPL=3） */
	uint32_t code_selector = (0 << 3) | (1 << 2) | 3; /* 索引 0, TI=1, RPL=3 */
	uint32_t data_selector = (1 << 3) | (1 << 2) | 3; /* 索引 1, TI=1, RPL=3 */

	/* 用户栈指针 */
	uint32_t user_esp_offset = header->data_size + header->bss_size + header->heap_size + header->stack_size;

	/* 释放文件缓冲区 */
	kfree(buf);
	buf = NULL;

	/* 启动服务 */
	asm volatile("lldt %0"::"m" (task->tss.ldtr));
	service_start(header->entry_point, code_selector, user_esp_offset, data_selector, &task->tss.esp0);

	/* 调用 SYSCALL_EXIT 后返回 */
	kfree(mem);
	return 0;

	/* 释放临时缓冲区 */
clean:
	if (mem) kfree(mem);
	if (buf) kfree(buf);

	return result;
}
