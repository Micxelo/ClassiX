/*
	include/ClassiX/assets.h
*/

#ifndef _CLASSIX_ASSETS_H_
#define _CLASSIX_ASSETS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef struct __attribute__((packed)) {
	uint32_t magic;			/* 签名 */
	union {
		uint32_t version;	/* 版本信息 */
		struct {
			uint16_t major;	/* 主版本号 */
			uint8_t minor;	/* 次版本号 */
			uint8_t patch;	/* 修订号 */
		};
	};
	uint32_t entry;			/* 入口点 */
	uint32_t size;			/* 内核大小 */
	uint32_t reserved[12];	/* 保留 */
} CLASSIX_HEADER;

extern const CLASSIX_HEADER *kernel_header;

/* 声明资源文件 */
#define DECLARE_ASSET(type, name) \
	extern const uint8_t binary_source_assets_##type##_##name##_start[];	\
	extern const uint8_t binary_source_assets_##type##_##name##_end[];		\
	extern const uint8_t binary_source_assets_##type##_##name##_size[];

/* 光标文件 */
DECLARE_ASSET(cursors, arrow_csr);
DECLARE_ASSET(cursors, beam_csr);
DECLARE_ASSET(cursors, busy_csr);
DECLARE_ASSET(cursors, link_csr);

/* 字体文件 */
DECLARE_ASSET(fonts, terminus12n_psf);
DECLARE_ASSET(fonts, terminus16n_psf);
DECLARE_ASSET(fonts, terminus16b_psf);

#undef DECLARE_ASSET

/* 引用资源数据指针 */
#define ASSET_DATA(type, name)				binary_source_assets_##type##_##name##_start

/* 获取资源大小 */
#define ASSET_SIZE(type, name) \
	((size_t) (binary_source_assets_##type##_##name##_end - binary_source_assets_##type##_##name##_start))

#ifdef __cplusplus
	}
#endif

#endif