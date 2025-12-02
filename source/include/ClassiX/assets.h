/*
	include/ClassiX/assets.h
*/

#ifndef _CLASSIX_ASSETS_H_
#define _CLASSIX_ASSETS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef uint32_t version_t;

/* 版本数值宏 */
#define VERSION(major, minor, patch) \
	((((major) & 0xFF) << 16) | (((minor) & 0xFF) << 8) | ((patch) & 0xFF))

/* 版本组件宏 */
#define VERSION_MAJOR(ver)					(((ver) >> 16) & 0xFF)	/* 主版本 */
#define VERSION_MINOR(ver)					(((ver) >> 8) & 0xFF)	/* 次版本 */
#define VERSION_PATCH(ver)					((ver) & 0xFF)			/* 修订版本 */

extern const version_t os_version;

/* 声明资源文件 */
#define DECLARE_ASSET(type, name) \
	extern const uint8_t binary_assets_##type##_##name##_start[];	\
	extern const uint8_t binary_assets_##type##_##name##_end[];		\
	extern const uint8_t binary_assets_##type##_##name##_size[];

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
#define ASSET_DATA(type, name)				binary_assets_##type##_##name##_start

/* 获取资源大小 */
#define ASSET_SIZE(type, name) \
	((size_t) (binary_assets_##type##_##name##_end - binary_assets_##type##_##name##_start))

#ifdef __cplusplus
	}
#endif

#endif