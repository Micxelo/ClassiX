/*
	ui/layer.h
*/

#include <ClassiX/debug.h>
#include <ClassiX/framebuf.h>
#include <ClassiX/layer.h>
#include <ClassiX/memory.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>

#include <string.h>

struct {
	uint32_t *fb;						/* 帧缓冲区 */
	uint8_t *map;
	uint16_t width, height;
	int32_t top;						/* 图层数量 */
	LAYER *layers[MAX_LAYERS];
	LAYER layers0[MAX_LAYERS];
} layer_manager = { };					/* 图层管理器 */

/*
	@brief 初始化图层管理器。
	@param fb 帧缓冲地址。
	@param width 帧缓冲宽度。
	@param height 帧缓冲高度。
	@return 成功返回 0，失败返回 -1。
*/
int32_t layer_init(uint32_t *fb, uint16_t width, uint16_t height)
{
	layer_manager.map = kmalloc(width * height * sizeof(uint8_t));
	if (!layer_manager.map) {
		debug("LAYER: Failed to allocate memory for layer manager.\n");
		return -1; /* 内存分配失败 */
	}

	memset(layer_manager.map, 0, width * height * sizeof(uint8_t));
	layer_manager.fb = fb;
	layer_manager.width = width;
	layer_manager.height = height;
	layer_manager.top = -1; /* 无图层 */

	for (int32_t i = 0; i < MAX_LAYERS; i++) {
		layer_manager.layers0[i].flags = LAYER_FREE;
		layer_manager.layers[i] = NULL;
	}

	debug("LAYER: Layer manager initialized.\n");
	return 0;
}

/*
	@brief 分配一个图层。
	@param width 图层宽度。
	@param height 图层高度。
	@param allow_inv 是否允许透明色。
	@return 成功返回图层指针，失败返回 NULL。
*/
LAYER *layer_alloc(uint16_t width, uint16_t height, bool allow_inv)
{
	LAYER *layer;

	for (int32_t i = 0; i < MAX_LAYERS; i++) {
		if (layer_manager.layers0[i].flags == LAYER_FREE) {
			layer = &layer_manager.layers0[i];
			layer->buf = kmalloc(width * height * sizeof(uint32_t));
			if (!layer->buf) {
				debug("LAYER: Failed to allocate memory for new layer.\n");
				return NULL;
			}

			layer->flags = LAYER_USED;
			layer->width = width;
			layer->height = height;
			layer->z = -1; /* 隐藏 */
			layer->allow_inv = allow_inv;

			debug("LAYER: Layer created at %p, size %dx%d.\n", layer, width, height);
			return layer;
		}
	}

	debug("LAYER: Failed to allocate free layer.\n");
	return NULL;
}

/*
	@brief 刷新图层映射。
	@param vx0 刷新区域左上角 X 坐标
	@param vy0 刷新区域左上角 Y 坐标
	@param vx1 刷新区域右下角 X 坐标
	@param vy1 刷新区域右下角 Y 坐标
	@param z0 起始 Z 顺序
*/
static void layer_refreshmap(int32_t vx0, int32_t vy0, int32_t vx1, int32_t vy1, int32_t z0)
{
	int32_t bx0, by0, bx1, by1;
	uint8_t id;
	LAYER *layer;

	if (vx0 < 0) vx0 = 0;
	if (vy0 < 0) vy0 = 0;
	if (vx1 > layer_manager.width) vx1 = layer_manager.width;
	if (vy1 > layer_manager.height) vy1 = layer_manager.height;

	for (int32_t z = z0; z <= layer_manager.top; z++) {
		layer = layer_manager.layers[z];
		id = layer - layer_manager.layers0;

		bx0 = vx0 - layer->x;
		by0 = vy0 - layer->y;
		bx1 = vx1 - layer->x;
		by1 = vy1 - layer->y;

		if (bx0 < 0) bx0 = 0;
		if (by0 < 0) by0 = 0;
		if (bx1 > layer->width) bx1 = layer->width;
		if (by1 > layer->height) by1 = layer->height;

		if (!layer->allow_inv) {
			/* 不使用透明色 */
			for (int32_t by = by0; by < by1; by++) {
				int32_t vy = layer->y + by;
				for (int32_t bx = bx0; bx < bx1; bx++) {
					int32_t vx = layer->x + bx;
					layer_manager.map[vy * layer_manager.width + vx] = id;
				}
			}
		} else {
			/* 使用透明色 */
			for (int32_t by = by0; by < by1; by++) {
				int32_t vy = layer->y + by;
				for (int32_t bx = bx0; bx < bx1; bx++) {
					int32_t vx = layer->x + bx;
					if (GET_PIXEL32(layer->buf, layer->width, bx, by).a == 0xff) {
						layer_manager.map[vy * layer_manager.width + vx] = id;
					}
				}
			}
		}
	}
}

/*
	@brief 刷新图层显示内容。
	@param vx0 刷新区域左上角 X 坐标
	@param vy0 刷新区域左上角 Y 坐标
	@param vx1 刷新区域右下角 X 坐标
	@param vy1 刷新区域右下角 Y 坐标
	@param z0 起始 Z 顺序
	@param z1 结束 Z 顺序
*/
static void layer_refreshsub(int32_t vx0, int32_t vy0, int32_t vx1, int32_t vy1, int32_t z0, int32_t z1)
{
	int32_t bx0, by0, bx1, by1;
	uint8_t id;
	LAYER *layer;

	/* 修正超过画面范围的值 */
	if (vx0 < 0) vx0 = 0;
	if (vy0 < 0) vy0 = 0;
	if (vx1 > layer_manager.width) vx1 = layer_manager.width;
	if (vy1 > layer_manager.height) vy1 = layer_manager.height;

	for (int32_t z = z0; z <= z1; z++) {
		layer = layer_manager.layers[z];
		id = layer - layer_manager.layers0;
		
		bx0 = vx0 - layer->x;
		by0 = vy0 - layer->y;
		bx1 = vx1 - layer->x;
		by1 = vy1 - layer->y;

		if (bx0 < 0) bx0 = 0;
		if (by0 < 0) by0 = 0;
		if (bx1 > layer->width) bx1 = layer->width;
		if (by1 > layer->height) by1 = layer->height;

		if (g_fb.argb_format)
			for (int32_t by = by0; by < by1; by++) {
				int32_t vy = layer->y + by;
				for (int32_t bx = bx0; bx < bx1; bx++) {
					int32_t vx = layer->x + bx;
					if (layer_manager.map[vy * layer_manager.width + vx] == id) {
						COLOR pixel = GET_PIXEL32(layer->buf, layer->width, bx, by);
						SET_PIXEL32(layer_manager.fb, layer_manager.width, vx, vy, pixel);
					}
				}
			}
		else
			for (int32_t by = by0; by < by1; by++) {
				int32_t vy = layer->y + by;
				for (int32_t bx = bx0; bx < bx1; bx++) {
					int32_t vx = layer->x + bx;
					if (layer_manager.map[vy * layer_manager.width + vx] == id) {
						COLOR pixel = GET_PIXEL32(layer->buf, layer->width, bx, by);
						set_pixel(vx, vy, pixel);
					}
				}
			}
	}
}

/*
	@brief 设置图层的 Z 顺序。
	@param layer 目标图层。
	@param z1 新的 Z 顺序。
*/
void layer_set_z(LAYER *layer, int32_t z1)
{
	int32_t z0 = layer->z;
	if (z0 == z1) return;

	/* 对超出范围的值进行修正 */
	if (z1 > layer_manager.top + 1) z1 = layer_manager.top + 1;
	if (z1 < -1) z1 = -1;
	layer->z = z1;

	/* 重新排列 layers[] */
	if (z0 > z1) {
		/* 调低 */
		if (z1 >= 0) {
			for (int32_t z = z0; z > z1; z--) {
				layer_manager.layers[z] = layer_manager.layers[z - 1];
				layer_manager.layers[z]->z = z;
			}
			layer_manager.layers[z1] = layer;
			layer_refreshmap(layer->x, layer->y, layer->x + layer->width, layer->y + layer->height, z1 + 1);
			layer_refreshsub(layer->x, layer->y, layer->x + layer->width, layer->y + layer->height, z1 + 1, z0);
		} else {
			if (z0 < layer_manager.top) {
				for (int32_t z = z0; z < layer_manager.top; z++) {
					layer_manager.layers[z] = layer_manager.layers[z + 1];
					layer_manager.layers[z]->z = z;
				}
			}
			layer_manager.top--;
			layer_refreshmap(layer->x, layer->y, layer->x + layer->width, layer->y + layer->height, 0);
			layer_refreshsub(layer->x, layer->y, layer->x + layer->width, layer->y + layer->height, 0, z0 - 1);
		}
	} else {
		/* 调高 */
		if (z0 >= 0) {
			for (int32_t z = z0; z < z1; z++) {
				layer_manager.layers[z] = layer_manager.layers[z + 1];
				layer_manager.layers[z]->z = z;
			}
			layer_manager.layers[z1] = layer;
		} else {
			for (int32_t z = layer_manager.top; z >= z1; z--) {
				layer_manager.layers[z + 1] = layer_manager.layers[z];
				layer_manager.layers[z + 1]->z = z + 1;
			}
			layer_manager.layers[z1] = layer;
			layer_manager.top++;
		}
		layer_refreshmap(layer->x, layer->y, layer->x + layer->width, layer->y + layer->height, z1);
		layer_refreshsub(layer->x, layer->y, layer->x + layer->width, layer->y + layer->height, z1, z1);
	}
}

/*
	@brief 刷新图层的部分区域。
	@param layer 目标图层。
	@param x0 区域左上角 X 坐标。
	@param y0 区域左上角 Y 坐标。
	@param x1 区域右下角 X 坐标。
	@param y1 区域右下角 Y 坐标。
*/
void layer_refresh(LAYER *layer, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	if (layer->z >= 0)
		layer_refreshsub(layer->x + x0, layer->y + y0, layer->x + x1, layer->y + y1, layer->z, layer->z);
}

/*
	@brief 移动图层位置。
	@param layer 目标图层。
	@param x 新的 X 坐标。
	@param y 新的 Y 坐标。
*/
void layer_move(LAYER *layer, int32_t x, int32_t y)
{
	int32_t x0 = layer->x, y0 = layer->y;
	layer->x = x;
	layer->y = y;

	if (layer->z >= 0) {
		layer_refreshmap(x0, y0, x0 + layer->width, y0 + layer->height, 0);
		layer_refreshmap(x, y, x + layer->width, y + layer->height, layer->z);
		layer_refreshsub(x0, y0, x0 + layer->width, y0 + layer->height, 0, layer->z - 1);
		layer_refreshsub(x, y, x + layer->width, y + layer->height, layer->z, layer->z);
	}
}

/*
	@brief 释放图层。
	@param layer 目标图层。
*/
void layer_free(LAYER *layer)
{
	if (layer->z >= 0)
		layer_set_z(layer, -1);
	
	layer->flags = LAYER_FREE;
	kfree(layer->buf);
	return;
}
