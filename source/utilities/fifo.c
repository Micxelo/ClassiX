/*
	utilities/fifo.c
*/

#include <ClassiX/fifo.h>
#include <ClassiX/typedef.h>

/*
	@brief 初始化 FIFO。
	@param fifo FIFO 结构体指针
	@param size FIFO 缓冲区大小
	@param buf FIFO 缓冲区指针
	@param task 有数据写入时需要唤醒的任务
*/
void fifo_init(FIFO *fifo, size_t size, uint32_t *buf, HANDLE *task)
{
	fifo->buf = buf;
	fifo->size = size;
	fifo->free = size;		/* 空 */
	fifo->idx_write = 0;	/* 读取位置 */
	fifo->idx_write = 0;	/* 写入位置 */
	fifo->task = task;		/* 有数据写入时需要唤醒的任务 */
}

/*
	@brief 向 FIFO 中写入数据。
	@param fifo FIFO 结构体指针
	@param data 要写入的数据
	@return 如果写入成功，返回 0；如果 FIFO 已满，返回 -1。
*/
int fifo_push(FIFO *fifo, uint32_t data)
{
	if (fifo->free == 0) {
		/* 无空间则溢出 */
		return -1;
	}

	fifo->buf[fifo->idx_write] = data;
	fifo->idx_write++;
	if ((size_t) fifo->idx_write == fifo->size) {
		fifo->idx_write = 0;
	}
	fifo->free--;
	// if (fifo->task != 0) {
	// 	if (fifo->task->flags != 2) {
	// 		/* 任务处于休眠状态 */
	// 		task_run(fifo->task, -1, 0); /* 唤醒任务 */
	// 	}
	// }
	return 0;
}

/*
	@brief 从 FIFO 中读取数据。
	@param fifo FIFO 结构体指针
	@return 如果读取成功，返回数据；如果 FIFO 为空，返回 -1。
*/
uint32_t fifo_pop(FIFO *fifo)
{
	uint32_t data;
	if (fifo->free == fifo->size) {
		/* 缓冲区为空则溢出 */
		return -1;
	}
	data = fifo->buf[fifo->idx_read];
	fifo->idx_read++;
	if ((size_t) fifo->idx_read == fifo->size) {
		fifo->idx_read = 0;
	}
	fifo->free++;
	return data;
}

/*
	@brief 获取 FIFO 中的数据数量。
	@param fifo FIFO 结构体指针
	@return FIFO 中的数据数量。
*/
static inline int fifo_size(FIFO *fifo)
{
	return fifo->size - fifo->free;
}
inline int fifo_status(FIFO *fifo)
{
	return fifo->size - fifo->free;
}
