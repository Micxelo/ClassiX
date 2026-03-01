/*
	utilities/fifo.c
*/

#include <ClassiX/fifo.h>
#include <ClassiX/io.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>
#include <ClassiX/window.h>

/*
	@brief 初始化 FIFO。
	@param fifo FIFO 结构体指针
	@param size FIFO 缓冲区大小
	@param buf FIFO 缓冲区指针
	@param task 有数据写入时需要唤醒的任务
*/
void fifo_init(FIFO *fifo, size_t size, uint32_t *buf, TASK *task)
{
	fifo->buf = buf;
	fifo->size = size;
	fifo->free = size;		/* 空 */
	fifo->idx_read = 0;		/* 读取位置 */
	fifo->idx_write = 0;	/* 写入位置 */
	fifo->task = task;		/* 有数据写入时需要唤醒的任务 */
}

/*
	@brief 向 FIFO 中写入数据。
	@param fifo FIFO 结构体指针
	@param data 要写入的数据
	@return 如果写入成功，返回 0；如果 FIFO 已满，返回 -1。
*/
int32_t fifo_push(FIFO *fifo, uint32_t data)
{
	if (fifo->free == 0)
		return -1; /* 无空间则溢出 */

	cli();
	fifo->buf[fifo->idx_write] = data;
	fifo->idx_write++;
	if ((size_t) fifo->idx_write == fifo->size)
		fifo->idx_write = 0;
	fifo->free--;
	sti();

	if (fifo->task)
		if (fifo->task->state != TASK_RUNNING) /* 任务处于休眠状态 */
			task_register(fifo->task, fifo->task->priority); /* 唤醒任务 */

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
	if (fifo->free == fifo->size)
		return -1; /* 缓冲区为空则溢出 */

	cli();
	data = fifo->buf[fifo->idx_read];
	fifo->idx_read++;
	if ((size_t) fifo->idx_read == fifo->size)
		fifo->idx_read = 0;

	fifo->free++;
	sti();

	return data;
}

/*
	@brief 获取 FIFO 中的数据数量。
	@param fifo FIFO 结构体指针
	@return FIFO 中的数据数量。
*/
int32_t fifo_status(FIFO *fifo)
{
	return fifo->size - fifo->free;
}

/*
	@brief 向 FIFO 中写入事件。
	@param fifo FIFO 结构体指针
	@param event 要写入的事件
	@return 如果写入成功，返回 0；如果 FIFO 已满，返回 -1。
*/
int32_t fifo_push_event(FIFO *fifo, const EVENT *event)
{
	if (fifo->free < 3)
		return -1;

	cli();
	fifo_push(fifo, (uint32_t) event->window);
	fifo_push(fifo, event->id);
	fifo_push(fifo, event->param);
	sti();
	return 0;
}

/*
	@brief 从 FIFO 中读取事件。
	@param fifo FIFO 结构体指针
	@param event 保存事件的指针
	@return 如果读取成功，返回 0；如果 FIFO 为空，返回 -1。
*/
int32_t fifo_pop_event(FIFO *fifo, EVENT *event)
{
	if (fifo_status(fifo) < 3)
		return -1;

	cli();
	event->window = (WINDOW *) fifo_pop(fifo);
	event->id = fifo_pop(fifo);
	event->param = fifo_pop(fifo);
	sti();
	return 0;
}
