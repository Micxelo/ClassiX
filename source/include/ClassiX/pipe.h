/*
	include/ClassiX/pipe.h
*/

#ifndef _CLASSIX_PIPE_H_
#define _CLASSIX_PIPE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/spinlock.h>
#include <ClassiX/typedef.h>

typedef struct TASK TASK;

typedef struct {
	uint8_t *buf;		/* 缓冲区 */
	size_t size;		/* 总容量 */
	size_t free;		/* 可用容量 */
	size_t pos_read;	/* 读索引 */
	size_t pos_write;	/* 写索引 */
	TASK *reader;		/* 等待的读任务 */
	TASK *writer;		/* 等待的写任务 */
	spinlock_t lock;	/* 自旋锁 */
} PIPE;

void pipe_init(PIPE *pipe, uint8_t *buf, size_t size);
size_t pipe_read(PIPE *pipe, uint8_t *buf, size_t count);
size_t pipe_write(PIPE *pipe, const uint8_t *buf, size_t count);

#ifdef __cplusplus
	}
#endif

#endif