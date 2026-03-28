/*
	include/ctype.h
*/

#ifndef _CTYPE_H_
#define _CTYPE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdint.h>

static inline int32_t isalpha(int32_t c);
static inline int32_t isdigit(int32_t c);

/*
	@brief 检查字符是否为字母或数字。
	@param c 要检查的字符
	@return 如果是字母或数字则返回非零值，否则返回零
*/
static inline int32_t isalnum(int32_t c)
{
	return isalpha(c) || isdigit(c);
}

/*
	@brief 检查字符是否为字母。
	@param c 要检查的字符
	@return 如果是字母则返回非零值，否则返回零
*/
static inline int32_t isalpha(int32_t c)
{
	return ((unsigned) c | 32) - 'a' < 26;
}

/*
	@brief 检查字符是否为空白字符（空格或制表符）。
	@param c 要检查的字符
	@return 如果是空白字符则返回非零值，否则返回零
*/
static inline int32_t isblank(int32_t c)
{
	return (c == ' ' || c == '\t');
}

/*
	@brief 检查字符是否为控制字符。
	@param c 要检查的字符
	@return 如果是控制字符则返回非零值，否则返回零
*/
static inline int32_t iscntrl(int32_t c)
{
	return (unsigned) c < 0x20 || c == 0x7f;
}

/*
	@brief 检查字符是否为数字。
	@param c 要检查的字符
	@return 如果是数字则返回非零值，否则返回零
*/
static inline int32_t isdigit(int32_t c)
{
	return (unsigned) c - '0' < 10;
}

/*
	@brief 检查字符是否为可打印字符（除空格外的所有可见字符）。
	@param c 要检查的字符
	@return 如果是可打印字符则返回非零值，否则返回零
*/
static inline int32_t isgraph(int32_t c)
{
	return (unsigned) c - 0x21 < 0x5e;
}

/*
	@brief 检查字符是否为小写字母。
	@param c 要检查的字符
	@return 如果是小写字母则返回非零值，否则返回零
*/
static inline int32_t islower(int32_t c)
{
	return (unsigned) c - 'a' < 26;
}

/*
	@brief 检查字符是否为可打印字符（包括空格）。
	@param c 要检查的字符
	@return 如果是可打印字符则返回非零值，否则返回零
*/
static inline int32_t isprint(int32_t c)
{
	return (unsigned) c - 0x20 < 0x5f;
}

/*
	@brief 检查字符是否为标点符号。
	@param c 要检查的字符
	@return 如果是标点符号则返回非零值，否则返回零
*/
static inline int32_t ispunct(int32_t c)
{
	return isgraph(c) && !isalnum(c);
}

/*
	@brief 检查字符是否为空格字符（空格、制表符、换行符等）。
	@param c 要检查的字符
	@return 如果是空格字符则返回非零值，否则返回零
*/
static inline int32_t isspace(int32_t c)
{
	return c == ' ' || (unsigned) c - '\t' < 5;
}

/*
	@brief 检查字符是否为大写字母。
	@param c 要检查的字符
	@return 如果是大写字母则返回非零值，否则返回零
*/
static inline int32_t isupper(int32_t c)
{
	return (unsigned) c - 'A' < 26;
}

/*
	@brief 检查字符是否为十六进制数字。
	@param c 要检查的字符
	@return 如果是十六进制数字则返回非零值，否则返回零
*/
static inline int32_t isxdigit(int32_t c)
{
	return isdigit(c) || ((unsigned) c | 32) - 'a' < 6;
}

/*
	@brief 将字符转换为小写字母。
	@param c 要转换的字符
	@return 转换后的字符
*/
static inline int32_t tolower(int32_t c)
{
	if (isupper(c))
		return c | 32;
	return c;
}

/*
	@brief 将字符转换为大写字母。
	@param c 要转换的字符
	@return 转换后的字符
*/
static inline int32_t toupper(int32_t c)
{
	if (islower(c))
		return c & 0x5f;
	return c;
}

#ifdef __cplusplus
	}
#endif

#endif
