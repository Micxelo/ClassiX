/*
	include/ClassiX/fifo.h
*/

#ifndef _CLASSIX_FIFO_H_
#define _CLASSIX_FIFO_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef struct TASK TASK;

typedef struct {
	uint32_t *buf;
	int idx_read, idx_write;
	size_t size, free;
	TASK *task;
} FIFO;

void fifo_init(FIFO *fifo, size_t size, uint32_t *buf, TASK *task);
int fifo_push(FIFO *fifo, uint32_t data);
uint32_t fifo_pop(FIFO *fifo);
int fifo_status(FIFO *fifo);

#ifdef __cplusplus
	}
#endif

#endif