/*
	utilities/pipe.c
*/

#include <ClassiX/pipe.h>
#include <ClassiX/task.h>
#include <ClassiX/typedef.h>

/*
	@brief 初始化管道。
	@param pipe 管道对象
	@param buf 缓冲区
	@param size 缓冲区大小
*/
void pipe_init(PIPE *pipe, uint8_t *buf, size_t size)
{
	pipe->buf = buf;
	pipe->size = size;
	pipe->free = size;
	pipe->pos_read = 0;
	pipe->pos_write = 0;
	pipe->reader = NULL;
	pipe->writer = NULL;
	spinlock_init(&pipe->lock);
}

/*
	@brief 从管道读取数据。
	@param pipe 管道对象
	@param buf 目标缓冲区
	@param count 要读取的字节数
	@return 实际读取的字节数
*/
size_t pipe_read(PIPE *pipe, uint8_t *buf, size_t count)
{
	uint32_t eflags = spinlock_acquire_irqsave(&pipe->lock);
	size_t read = 0; /* 实际读取的字节数 */

	while (count > 0) {
		if (pipe->free == pipe->size) {
			/* 缓冲区为空 */
			TASK *current = task_get_current();
			pipe->reader = current;
			spinlock_release_irqrestore(&pipe->lock, eflags);
			task_sleep(current);
			eflags = spinlock_acquire_irqsave(&pipe->lock);
			continue;
		}

		/* 计算本次可读取的字节数 */
		size_t to_read = (count < (pipe->size - pipe->free)) ? count : (pipe->size - pipe->free);
		for (size_t i = 0; i < to_read; i++) {
			buf[read + i] = pipe->buf[pipe->pos_read];
			pipe->pos_read = (pipe->pos_read + 1) % pipe->size;
		}
		pipe->free += to_read;
		read += to_read;
		count -= to_read;

		/* 唤醒等待写入的任务 */
		if (pipe->writer && pipe->free > 0) {
			task_register(pipe->writer, PRIORITY_NORMAL);
			pipe->writer = NULL;
		}
	}

	spinlock_release_irqrestore(&pipe->lock, eflags);
	return read;
}

/*
	@brief 向管道写入数据。
	@param pipe 管道对象
	@param buf 源缓冲区
	@param count 要写入的字节数
	@return 实际写入的字节数
*/
size_t pipe_write(PIPE *pipe, const uint8_t *buf, size_t count)
{
	uint32_t eflags = spinlock_acquire_irqsave(&pipe->lock);
	size_t written = 0; /* 实际写入的字节数 */

	while (count > 0) {
		if (pipe->free == 0) {
			/* 缓冲区已满 */
			TASK *current = task_get_current();
			pipe->writer = current;
			spinlock_release_irqrestore(&pipe->lock, eflags);
			task_sleep(current);
			eflags = spinlock_acquire_irqsave(&pipe->lock);
			continue;
		}

		/* 计算本次可写入的字节数 */
		size_t to_write = (count < pipe->free) ? count : pipe->free;
		for (size_t i = 0; i < to_write; i++) {
			pipe->buf[pipe->pos_write] = buf[written + i];
			pipe->pos_write = (pipe->pos_write + 1) % pipe->size;
		}
		pipe->free -= to_write;
		written += to_write;
		count -= to_write;

		/* 唤醒等待读取的任务 */
		if (pipe->reader && pipe->free < pipe->size) {
			task_register(pipe->reader, PRIORITY_NORMAL);
			pipe->reader = NULL;
		}
	}

	spinlock_release_irqrestore(&pipe->lock, eflags);
	return written;
}
