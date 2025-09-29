#/*
	include/ClassiX/rtc.h
*/

#ifndef _CLASSIX_RTC_H_
#define _CLASSIX_RTC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <ClassiX/typedef.h>

uint32_t get_hour(void);
void set_hour(uint32_t hour_hex);
uint32_t get_minute(void);
void set_minute(uint32_t min_hex);
uint32_t get_second(void);
void set_second(uint32_t sec_hex);
uint32_t get_day_of_month(void);
void set_day_of_month(uint32_t day_of_month);
uint32_t get_day_of_week(void);
void set_day_of_week(uint32_t day_of_week);
uint32_t get_month(void);
void set_month(uint32_t mon_hex);
uint32_t get_year(void);
void set_year(uint32_t year);
uint64_t timestamp(int32_t year, int32_t mon, int32_t day, int32_t hour, int32_t min, int32_t sec);

#ifdef __cplusplus
	}
#endif

#endif
