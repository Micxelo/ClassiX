/*
	include/ClassiX/assets.h
*/

#ifndef _CLASSIX_ASSETS_H_
#define _CLASSIX_ASSETS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#define MAJOR
#define MINOR
#define PATCH

/* 声明资源文件 */
#define DECLARE_ASSET(type, name) \
	extern const uint8_t binary_assets_##type##_##name##_start[];	\
	extern const uint8_t binary_assets_##type##_##name##_end[];	\
	extern const uint8_t binary_assets_##type##_##name##_size[];

/* 光标文件 */
DECLARE_ASSET(cursors, arrow_csr);
DECLARE_ASSET(cursors, busy_csr);
DECLARE_ASSET(cursors, link_csr);

/* 字体文件 */
DECLARE_ASSET(fonts, terminus12n_psf);
DECLARE_ASSET(fonts, terminus16n_psf);
DECLARE_ASSET(fonts, terminus16b_psf);

#undef DECLARE_ASSET

/* 引用资源数据指针 */
#define ASSET_DATA(type, name)				binary_assets_##type##_##name##_start

/* 获取资源大小 */
#define ASSET_SIZE(type, name) \
	((size_t) (binary_assets_##type##_##name##_end - binary_assets_##type##_##name##_start))

#ifdef __cplusplus
	}
#endif

#endif