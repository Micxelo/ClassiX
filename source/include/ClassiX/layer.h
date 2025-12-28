/*
	include/ClassiX/layer.h
*/

#ifndef _CLASSIX_LAYER_H_
#define _CLASSIX_LAYER_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

#include <stdint.h>

#define MAX_LAYER_IDX						(UINT8_MAX)			/* 最大图层下标 */
#define MAX_LAYERS							(MAX_LAYER_IDX + 1)	/* 最大图层数量 */

#define LAYER_FREE							(0)
#define LAYER_USED							(1)

typedef struct {
	uint32_t *buf;
	uint16_t width, height;
	int16_t x, y;
	int32_t z;					/* Z 高度 */
	uint32_t flags;
	bool allow_inv;				/* 是否启用透明色 */
	struct WINDOW *window;		/* 关联的窗口 */
} LAYER;

typedef struct {
	uint32_t *fb;				/* 帧缓冲区 */
	uint8_t *map;
	uint16_t width, height;
	int32_t top;				/* 图层数量 */
	LAYER *layers[MAX_LAYERS];
	LAYER layers0[MAX_LAYERS];
} LAYER_MANAGER;

/* 全局图层管理器 */
extern LAYER_MANAGER g_lm;

int32_t layer_init(uint32_t *fb, uint16_t width, uint16_t height);
LAYER *layer_alloc(uint16_t width, uint16_t height, bool allow_inv);
void layer_set_z(LAYER *layer, int32_t z1);
void layer_refresh(LAYER *layer, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
void layer_move(LAYER *layer, int32_t x, int32_t y);
void layer_free(LAYER *layer);

#ifdef __cplusplus
	}
#endif

#endif
