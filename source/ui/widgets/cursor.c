/*
	ui/widgets/cursor.c
*/

#include <ClassiX/assets.h>
#include <ClassiX/graphic.h>
#include <ClassiX/layer.h>
#include <ClassiX/palette.h>
#include <ClassiX/typedef.h>
#include <ClassiX/widgets.h>

#include <string.h>

#define CSR_MAGIC							(0xc2d36fb8)

/* CSR 文件头 */
typedef struct __attribute__((packed)) {
	uint32_t magic;			/* 魔数 */
	uint16_t width;			/* 光标宽度 */
	uint16_t height;		/* 光标高度 */
	uint16_t anchor_x;		/* 判定点 X */
	uint16_t anchor_y;		/* 判定点 Y */
	uint32_t reserved;
} CSR_HEADER;

static LAYER *cursor_layer;
static CSR_HEADER *current;

static CSR_HEADER *cursor_data[] = {
	(CSR_HEADER *) ASSET_DATA(cursors, arrow_csr),
	(CSR_HEADER *) ASSET_DATA(cursors, beam_csr),
	(CSR_HEADER *) ASSET_DATA(cursors, busy_csr),
	(CSR_HEADER *) ASSET_DATA(cursors, link_csr)
};

/*
	@brief 初始化鼠标光标。
	@return 光标图层指针，失败返回 NULL
*/
LAYER *cursor_init(void)
{
	current = cursor_data[CURSOR_ARROW];
	if (current->magic != CSR_MAGIC)
		return NULL;

	cursor_layer = layer_alloc(current->width, current->height, true);
	if (!cursor_layer)
		return NULL;

	memcpy(cursor_layer->buf, (uint8_t *) (current + 1), current->width * current->height * sizeof(COLOR));

	return cursor_layer;
}

/*
	@brief 设置鼠标光标形状。
	@param type 光标类型
*/
void cursor_set(CURSOR type)
{
	CSR_HEADER *csr;

	if (type >= sizeof(cursor_data) / sizeof(cursor_data[0]))
		return;

	csr = (CSR_HEADER *) cursor_data[type];
	if (csr->magic != CSR_MAGIC)
		return;

	current = csr;

	memcpy(cursor_layer->buf, (uint8_t *) (current + 1), current->width * current->height * sizeof(COLOR));
}
