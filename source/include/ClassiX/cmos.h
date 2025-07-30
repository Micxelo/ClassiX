/*
	include/ClassiX/cmos.h
*/

#ifndef _CMOS_H_
#define _CMOS_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>
#include <ClassiX/io.h>

#define CMOS_INDEX								0x70
#define CMOS_DATA								0x71
#define CMOS_CUR_SEC							0x00
#define CMOS_ALA_SEC							0x01
#define CMOS_CUR_MIN							0x02
#define CMOS_ALA_MIN							0x03
#define CMOS_CUR_HOUR							0x04
#define CMOS_ALA_HOUR							0x05
#define CMOS_WEEK_DAY							0x06
#define CMOS_MON_DAY							0x07
#define CMOS_CUR_MON							0x08
#define CMOS_CUR_YEAR							0x09
#define CMOS_DEV_TYPE							0x12
#define CMOS_CUR_CEN							0x32

#define cmos_read(port) ({						\
	uint8_t _data;								\
	out8(CMOS_INDEX, port);						\
	_data = in8(CMOS_DATA);					\
	out8(CMOS_INDEX, 0x80);						\
	_data; })

#define cmos_write(port, data) ({				\
	out8(CMOS_INDEX, port);						\
	out8(CMOS_DATA, data); })

#ifdef __cplusplus
	}
#endif

#endif