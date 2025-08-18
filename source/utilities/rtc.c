/*
	utilities/rtc.c
*/

#include <ClassiX/cmos.h>
#include <ClassiX/rtc.h>
#include <ClassiX/typedef.h>

#define BCD_HEX(n)							((n >> 4) * 10) + (n & 0x0f)
#define HEX_BCD(n)  						((n / 10) << 4) + (n % 10)
#define BCD_ASCII_FIRST(n)					(((n << 4) >> 4) + 0x30)
#define BCD_ASCII_SECOND(n)					((n << 4) + 0x30)

/*
	@brief 获取当前时。
	@return 当前时
*/
uint32_t get_hour(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_HOUR));
}

/*
	@brief 设置当前时。
	@param hour_hex 时
*/
inline void set_hour(uint32_t hour_hex)
{
	cmos_write(CMOS_CUR_HOUR, HEX_BCD(hour_hex));
}

/*
	@brief 获取当前分钟。
	@return 当前分钟
*/
uint32_t get_minute(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_MIN));
}

/*
	@brief 设置当前分钟。
	@param min_hex 分钟
*/
inline void set_minute(uint32_t min_hex)
{
	cmos_write(CMOS_CUR_MIN, HEX_BCD(min_hex));
}

/*
	@brief 获取当前秒。
	@return 当前秒
*/
uint32_t get_second(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_SEC));
}

/*
	@brief 设置当前秒。
	@param sec_hex 秒
*/
inline void set_second(uint32_t sec_hex)
{
	cmos_write(CMOS_CUR_SEC, HEX_BCD(sec_hex));
}

/*
	@brief 获取当前日期（天）。
	@return 当前日期（天）
*/
uint32_t get_day_of_month(void)
{
	return BCD_HEX(cmos_read(CMOS_MON_DAY));
}

/*
	@brief 设置当前日期（天）。
	@param day_of_month 日期（天）
*/
inline void set_day_of_month(uint32_t day_of_month)
{
	cmos_write(CMOS_MON_DAY, HEX_BCD(day_of_month));
}

/*
	@brief 获取当前星期。
	@return 当前星期（0 - 6，0 表示星期日）
*/
uint32_t get_day_of_week(void)
{
	return BCD_HEX(cmos_read(CMOS_WEEK_DAY));
}

/*
	@brief 设置当前星期。
	@param day_of_week 星期（0 - 6，0 表示星期日）
*/
inline void set_day_of_week(uint32_t day_of_week)
{
	cmos_write(CMOS_WEEK_DAY, HEX_BCD(day_of_week));
}

/*
	@brief 获取当前月份。
	@return 当前月份（1 - 12）
*/
uint32_t get_month(void)
{
	return BCD_HEX(cmos_read(CMOS_CUR_MON));
}

/*
	@brief 设置当前月份。
	@param mon_hex 月份（1 - 12）
*/
inline void set_month(uint32_t mon_hex)
{
	cmos_write(CMOS_CUR_MON, HEX_BCD(mon_hex));
}

/*
	@brief 获取当前年份。
	@return 当前年份
*/
uint32_t get_year(void)
{
	return (BCD_HEX(cmos_read(CMOS_CUR_CEN)) * 100) + BCD_HEX(cmos_read(CMOS_CUR_YEAR)) + 1980;
}

/*
	@brief 设置当前年份。
	@param year 年份
*/
inline void set_year(uint32_t year)
{
	cmos_write(CMOS_CUR_CEN, HEX_BCD(year / 100));
	cmos_write(CMOS_CUR_YEAR, HEX_BCD((year - 2000)));
	return;
}

/*
	@brief 获取当前时间戳（秒）。
	@return 当前时间戳（秒）
*/
inline uint64_t mktime(int32_t year0, int32_t mon0, int32_t day, int32_t hour, int32_t min, int32_t sec)
{
	uint64_t mon = mon0, year = year0;

	if (0 >= (uint64_t) (mon -= 2)) {
		mon += 12;
		year -= 1;
	}
	return ((((uint64_t) (year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) + year * 365 - 719499) * 24 + hour )
		* 60 + min ) * 60 + sec;
}
