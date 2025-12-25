/*
	include/ctype.h
*/

#ifndef _CTYPE_H_
#define _CTYPE_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdint.h>

/* ASCII 字符分类表 */
static const uint16_t _ctype_table[256] = {
	0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
	0x0020, 0x0120, 0x0120, 0x0120, 0x0120, 0x0120, 0x0020, 0x0020,
	0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
	0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020,
	0x00B0, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
	0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
	0x0204, 0x0204, 0x0204, 0x0204, 0x0204, 0x0204, 0x0204, 0x0204,
	0x0204, 0x0204, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
	0x0200, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241,
	0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241,
	0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241, 0x0241,
	0x0241, 0x0241, 0x0241, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
	0x0200, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242,
	0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242,
	0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242, 0x0242,
	0x0242, 0x0242, 0x0242, 0x0200, 0x0200, 0x0200, 0x0200, 0x0020,
	/* 扩展 ASCII 字符 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

/* 位标志定义 */
#define _CTYPE_U						0x01	/* 大写字母 */
#define _CTYPE_L						0x02	/* 小写字母 */
#define _CTYPE_N						0x04	/* 数字 */
#define _CTYPE_S						0x08	/* 空格字符 */
#define _CTYPE_P						0x10	/* 标点符号 */
#define _CTYPE_C						0x20	/* 控制字符 */
#define _CTYPE_X						0x40	/* 十六进制数字 */
#define _CTYPE_B						0x80	/* 空白字符（空格和制表符） */

/*
	@brief 检查字符是否为字母或数字。
	@param c 要检查的字符
	@return 如果是字母或数字则返回非零值，否则返回零
*/
static inline int32_t isalnum(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & (_CTYPE_U | _CTYPE_L | _CTYPE_N));
}

/*
	@brief 检查字符是否为字母。
	@param c 要检查的字符
	@return 如果是字母则返回非零值，否则返回零
*/
static inline int32_t isalpha(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & (_CTYPE_U | _CTYPE_L));
}

/*
	@brief 检查字符是否为空白字符（空格或制表符）。
	@param c 要检查的字符
	@return 如果是空白字符则返回非零值，否则返回零
*/
static inline int32_t isblank(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_B);
}

/*
	@brief 检查字符是否为控制字符。
	@param c 要检查的字符
	@return 如果是控制字符则返回非零值，否则返回零
*/
static inline int32_t iscntrl(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_C);
}

/*
	@brief 检查字符是否为数字。
	@param c 要检查的字符
	@return 如果是数字则返回非零值，否则返回零
*/
static inline int32_t isdigit(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_N);
}

/*
	@brief 检查字符是否为可打印字符（除空格外的所有可见字符）。
	@param c 要检查的字符
	@return 如果是可打印字符则返回非零值，否则返回零
*/
static inline int32_t isgraph(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & (_CTYPE_U | _CTYPE_L | _CTYPE_N | _CTYPE_P));
}

/*
	@brief 检查字符是否为小写字母。
	@param c 要检查的字符
	@return 如果是小写字母则返回非零值，否则返回零
*/
static inline int32_t islower(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_L);
}

/*
	@brief 检查字符是否为可打印字符（包括空格）。
	@param c 要检查的字符
	@return 如果是可打印字符则返回非零值，否则返回零
*/
static inline int32_t isprint(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & (_CTYPE_U | _CTYPE_L | _CTYPE_N | _CTYPE_P | _CTYPE_B));
}

/*
	@brief 检查字符是否为标点符号。
	@param c 要检查的字符
	@return 如果是标点符号则返回非零值，否则返回零
*/
static inline int32_t ispunct(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_P);
}

/*
	@brief 检查字符是否为空格字符（空格、制表符、换行符等）。
	@param c 要检查的字符
	@return 如果是空格字符则返回非零值，否则返回零
*/
static inline int32_t isspace(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_S);
}

/*
	@brief 检查字符是否为大写字母。
	@param c 要检查的字符
	@return 如果是大写字母则返回非零值，否则返回零
*/
static inline int32_t isupper(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_U);
}

/*
	@brief 检查字符是否为十六进制数字。
	@param c 要检查的字符
	@return 如果是十六进制数字则返回非零值，否则返回零
*/
static inline int32_t isxdigit(int32_t c)
{
	return (c >= 0 && c < 256) && (_ctype_table[c] & _CTYPE_X);
}

/*
	@brief 将字符转换为小写字母。
	@param c 要转换的字符
	@return 转换后的字符
*/
static inline int32_t tolower(int32_t c)
{
	if (isupper(c))
		return c + ('a' - 'A');
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
		return c - ('a' - 'A');
	return c;
}

#undef _CTYPE_U
#undef _CTYPE_L
#undef _CTYPE_N
#undef _CTYPE_S
#undef _CTYPE_P
#undef _CTYPE_C
#undef _CTYPE_X
#undef _CTYPE_B

#ifdef __cplusplus
	}
#endif

#endif
