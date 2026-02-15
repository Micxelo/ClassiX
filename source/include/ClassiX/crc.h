/*
	include/ClassiX/crc.h
*/

#ifndef _CLASSIX_CRC_H_
#define _CLASSIX_CRC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

uint32_t crc32(const void *buf, size_t size);

#ifdef __cplusplus
	}
#endif

#endif
