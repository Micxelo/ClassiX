/*
	include/ClassiX/fifo.h
*/

#ifndef _FIFO_H_
#define _FIFO_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

typedef struct {
	uint32_t *buf;
	int idx_read, idx_write;
	size_t size, free;
	HANDLE task;
} FIFO;

void fifo_init(FIFO *fifo, size_t size, uint32_t *buf, HANDLE task);
int fifo_push(FIFO *fifo, uint32_t data);
uint32_t fifo_pop(FIFO *fifo);
int fifo_status(FIFO *fifo);

#ifdef __cplusplus
	}
#endif

#endif