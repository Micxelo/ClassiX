/*
	utilities/rtc.c
*/

#include <ClassiX/typedef.h>
#include <ClassiX/cmos.h>
#include <ClassiX/rtc.h>

#define BCD_HEX(n)							((n >> 4) * 10) + (n & 0x0f)
#define HEX_BCD(n)  						((n / 10) << 4) + (n % 10)
#define BCD_ASCII_FIRST(n)					(((n << 4) >> 4) + 0x30)
#define BCD_ASCII_SECOND(n)					((n << 4) + 0x30)

uint32_t get_hour(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_HOUR));
}

inline void set_hour(uint32_t hour_hex)
{
	cmos_write(CMOS_CUR_HOUR, HEX_BCD(hour_hex));
}

uint32_t get_minute(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_MIN));
}

inline void set_minute(uint32_t min_hex)
{
	cmos_write(CMOS_CUR_MIN, HEX_BCD(min_hex));
}

uint32_t get_second(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_SEC));
}

inline void set_second(uint32_t sec_hex)
{
	cmos_write(CMOS_CUR_SEC, HEX_BCD(sec_hex));
}

uint32_t get_day_of_month(void)
{
	return BCD_HEX(cmos_read(CMOS_MON_DAY));
}

inline void set_day_of_month(uint32_t day_of_month)
{
	cmos_write(CMOS_MON_DAY, HEX_BCD(day_of_month));
}

uint32_t get_day_of_week(void)
{
	return BCD_HEX(cmos_read(CMOS_WEEK_DAY));
}

inline void set_day_of_week(uint32_t day_of_week)
{
	cmos_write(CMOS_WEEK_DAY, HEX_BCD(day_of_week));
}

uint32_t get_month(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_MON));
}

inline void set_month(uint32_t mon_hex)
{
	cmos_write(CMOS_CUR_MON, HEX_BCD(mon_hex));
}

uint32_t get_year(void)
{
	return (BCD_HEX(cmos_read(CMOS_CUR_CEN)) * 100) + BCD_HEX(cmos_read(CMOS_CUR_YEAR)) + 1980;
}

inline void set_year(uint32_t year)
{
	cmos_write(CMOS_CUR_CEN, HEX_BCD(year / 100));
	cmos_write(CMOS_CUR_YEAR, HEX_BCD((year - 2000)));
	return;
}

inline uint32_t mktime(int32_t year0, int32_t mon0, int32_t day, int32_t hour, int32_t min, int32_t sec)
{
	uint32_t mon = mon0, year = year0;

	if (0 >= (int32_t) (mon -= 2)) {
		mon += 12;
		year -= 1;
	}
	return ((((uint32_t) (year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) + year * 365 - 719499) * 24 + hour )
		* 60 + min ) * 60 + sec;
}
