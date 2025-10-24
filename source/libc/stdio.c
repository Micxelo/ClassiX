/*
	libc/stdio/stdio.c
*/

/*
	public domain snprintf() implementation
	originally by Jeff Roberts / RAD Game Tools, 2015/10/20
	http://github.com/nothings/stb
*/

/*
	MIT License
	Copyright (c) 2017 Sean Barrett

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

/*
	附加说明

	支持的数据类型	sc uidBboXx p AaGgEef n
	长度			hh h ll j z t I64 I32 I

	对于整数和浮点数，可以使用“'”插入千位分隔符。
	例如，对于“12345”，“%'d”将打印“12,345”。

	对于整数和浮点数，可以使用“$”将数值转换为浮点型，并以千（kilo）、兆（mega）、
	吉（giga）或太（tera）换算。
	例如，对于“1000”，“%$d”将打印“1.0 k”；对于“2536000”，“$$.2d”将打印“2.53 M”。
	如需移除数值与单位间的空格，添加“_”，如“%_$d”。
	$：SI 标准，1000 进制，显示为“K”“M”；
	$$：二进制，1024 进制，显示为“Ki”“Mi”；
	$$$：JEDEC 标准，1024 进制，显示为“K”“M”。

	如同“%x”“%o”，“%b”将打印二进制整数。
	例如，对于“4”，“%b”将打印“100”。
*/

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define _SPRINTF_MIN 						512 /* 每次回调的字符数 */

typedef char *_SPRINTFCB(const char *buf, void *user, int32_t len);

int32_t vsprintf(char *buf, char const *fmt, va_list va);
int32_t vsnprintf(char *buf, int32_t count, char const *fmt, va_list va);
int32_t sprintf(char *buf, char const *fmt, ...) __attribute__((format(printf, 2, 3)));
int32_t snprintf(char *buf, int32_t count, char const *fmt, ...) __attribute__((format(printf, 3, 4)));

static int32_t _vsprintfcb(_SPRINTFCB *callback, void *user, char *buf, char const *fmt, va_list va);

static int32_t _real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits);
static int32_t _real_to_parts(int64_t *bits, int32_t *expo, double value);
#define _SPECIAL							0x7000

const char _period = '.';
const char _comma = ',';

static struct {
	int32_t temp; /* 使 pair 按 2 字节对齐 */
	char pair[201];
} _digitpair = {
	0,
	"00010203040506070809101112131415161718192021222324"
	"25262728293031323334353637383940414243444546474849"
	"50515253545556575859606162636465666768697071727374"
	"75767778798081828384858687888990919293949596979899"
};

#define _LEFTJUST							0x0001
#define _LEADINGPLUS						0x0002
#define _LEADINGSPACE						0x0004
#define _LEADING_0X							0x0008
#define _LEADINGZERO						0x0010
#define _INTMAX								0x0020
#define _TRIPLET_COMMA						0x0040
#define _NEGATIVE							0x0080
#define _METRIC_SUFFIX						0x0100
#define _HALFWIDTH							0x0200
#define _METRIC_NOSPACE						0x0400
#define _METRIC_1024						0x0800
#define _METRIC_JEDEC						0x1000

uint64_t __udivmoddi4(uint64_t dividend, uint64_t divisor, uint64_t *remainder) {
	if (divisor == 0) {
		if (remainder) *remainder = 0;
		return 0;
	}

	uint64_t quotient = 0;
	uint64_t current_remainder = 0;

	for (int32_t i = 0; i < 64; i++) {
		/* 左移余数并移入被除数的最高位 */
		current_remainder = (current_remainder << 1) | (dividend >> 63);
		dividend <<= 1;
		quotient <<= 1;

		/* 如果余数 >= 除数，则减去除数并设置商的最低位 */
		if (current_remainder >= divisor) {
			current_remainder -= divisor;
			quotient |= 1;
		}
	}

	if (remainder) *remainder = current_remainder;
	return quotient;
}

uint64_t __udivdi3(uint64_t dividend, uint64_t divisor) {
	return __udivmoddi4(dividend, divisor, NULL);
}

int64_t __divmoddi4(int64_t dividend, int64_t divisor, int64_t *remainder) {
	if (divisor == 0) {
		if (remainder) *remainder = 0;
		return 0;
	}

	/* 处理溢出：INT64_MIN / -1 */
	if (dividend == INT64_MIN && divisor == -1) {
		if (remainder) *remainder = 0;
		return INT64_MIN;
	}

	/* 计算符号 */
	const int32_t sign_dividend = (dividend < 0);
	const int32_t sign_divisor = (divisor < 0);
	const int32_t sign_quotient = sign_dividend ^ sign_divisor;
	const int32_t sign_remainder = sign_dividend;

	/* 转换为无符号绝对值 */
	uint64_t abs_dividend = sign_dividend ? (uint64_t)(-dividend) : (uint64_t)dividend;
	uint64_t abs_divisor = sign_divisor ? (uint64_t)(-divisor) : (uint64_t)divisor;

	/* 执行无符号除法 / 取模 */
	uint64_t abs_remainder;
	uint64_t abs_quotient = __udivmoddi4(abs_dividend, abs_divisor, &abs_remainder);

	/* 应用符号到结果 */
	int64_t final_quotient = sign_quotient ? -(int64_t)abs_quotient : (int64_t)abs_quotient;
	if (remainder) {
		*remainder = sign_remainder ? -(int64_t)abs_remainder : (int64_t)abs_remainder;
	}

	return final_quotient;
}

static void _lead_sign(uint32_t fl, char *sign)
{
	sign[0] = 0;
	if (fl & _NEGATIVE) {
		sign[0] = 1;
		sign[1] = '-';
	} else if (fl & _LEADINGSPACE) {
		sign[0] = 1;
		sign[1] = ' ';
	} else if (fl & _LEADINGPLUS) {
		sign[0] = 1;
		sign[1] = '+';
	}
}

static uint32_t _strlen_limited(char const *s, uint32_t limit)
{
	char const *sn = s;

	/* 获取 4 字节对齐的内存地址 */
	for (;;) {
		if (((uintptr_t)sn & 3) == 0)
			break;

		if (!limit || *sn == 0)
			return (uint32_t)(sn - s);

		++sn;
		--limit;
	}

	while (limit >= 4) {
		uint32_t v = *(uint32_t *)sn;
		/* 检查是否存在 0 字节 */
		if ((v - 0x01010101) & (~v) & 0x80808080UL)
			break;

		sn += 4;
		limit -= 4;
	}

	/* 确定实际长度 */
	while (limit && *sn) {
		++sn;
		--limit;
	}

	return (uint32_t)(sn - s);
}

static int32_t _vsprintfcb(_SPRINTFCB *callback, void *user, char *buf, char const *fmt, va_list va)
{
	static char hex[] = "0123456789abcdefxp";
	static char hexu[] = "0123456789ABCDEFXP";
	char *bf;
	char const *f;
	int32_t tlen = 0;

	bf = buf;
	f = fmt;
	for (;;) {
		int32_t fw, pr, tz;
		uint32_t fl;

/* 用于回调缓冲区的宏 */
#define _chk_cb_bufL(bytes) {								\
		int32_t len = (int32_t)(bf - buf);							\
		if ((len + (bytes)) >= _SPRINTF_MIN) {				\
			tlen += len;									\
			if (0 == (bf = buf = callback(buf, user, len)))	\
				goto done;									\
		}													\
	}
#define _chk_cb_buf(bytes) {		\
		if (callback) {				\
			_chk_cb_bufL(bytes);	\
		}							\
	}
#define _flush_cb() {								\
		_chk_cb_bufL(_SPRINTF_MIN - 1);				\
	} /* 缓冲区非空则刷新 */
#define _cb_buf_clamp(cl, v)						\
	cl = v;											\
	if (callback) {									\
		int32_t lg = _SPRINTF_MIN - (int32_t)(bf - buf);	\
		if (cl > lg)								\
			cl = lg;								\
	}

		/* 快速复制，直至遇到下一个“%”或字符串末尾 */
		for (;;) {
			while (((uintptr_t)f) & 3) {
			schk1:
				if (f[0] == '%')
					goto scandd;
			schk2:
				if (f[0] == 0)
					goto endfmt;
				_chk_cb_buf(1);
				*bf++ = f[0];
				++f;
			}
			for (;;) {
				/* 检查接下来的 4 字节中是否含有“%”（0x25）或字符串末尾 */
				/* 使用“hasless” */
				/* https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord */
				uint32_t v, c;
				v = *(uint32_t *)f;
				c = (~v) & 0x80808080;
				if (((v ^ 0x25252525) - 0x01010101) & c)
					goto schk1;
				if ((v - 0x01010101) & c)
					goto schk2;
				if (callback)
					if ((_SPRINTF_MIN - (int32_t)(bf - buf)) < 4)
						goto schk1;
				if (((uintptr_t)bf) & 3) {
					bf[0] = f[0];
					bf[1] = f[1];
					bf[2] = f[2];
					bf[3] = f[3];
				} else {
					*(uint32_t *)bf = v;
				}
				bf += 4;
				f += 4;
			}
		}
	scandd:

		++f;

		/* 出现“%”，读取修饰符 */
		fw = 0;
		pr = -1;
		fl = 0;
		tz = 0;

		for (;;) {
			switch (f[0]) {
				case '-':	/* 是否左对齐 */
					fl |= _LEFTJUST;
					++f;
					continue;
				case '+':	/* 是否有前导“+” */
					fl |= _LEADINGPLUS;
					++f;
					continue;
				case ' ':	/* 是否有前导空格 */
					fl |= _LEADINGSPACE;
					++f;
					continue;
				case '#':	/* 是否有前导“0x” */
					fl |= _LEADING_0X;
					++f;
					continue;
				case '\'':	/* 是否有千位分隔符 */
					fl |= _TRIPLET_COMMA;
					++f;
					continue;
				case '$':	/* 是否有千位标记 */
					if (fl & _METRIC_SUFFIX) {
						if (fl & _METRIC_1024) {
							fl |= _METRIC_JEDEC;
						} else {
							fl |= _METRIC_1024;
						}
					} else {
						fl |= _METRIC_SUFFIX;
					}
					++f;
					continue;
				case '_':	/* 是否去除数字与 SI 后缀间的空格 */
					fl |= _METRIC_NOSPACE;
					++f;
					continue;
				case '0':	/* 是否有前导 0 */
					fl |= _LEADINGZERO;
					++f;
					goto flags_done;
				default:
					goto flags_done;
			}
		}

	flags_done:
		/* 获取字符宽度 */
		if (f[0] == '*') {
			fw = va_arg(va, uint32_t);
			++f;
		} else {
			while ((f[0] >= '0') && (f[0] <= '9')) {
				fw = fw * 10 + f[0] - '0';
				f++;
			}
		}
		/* 获取精确度 */
		if (f[0] == '.') {
			++f;
			if (f[0] == '*') {
				pr = va_arg(va, uint32_t);
				++f;
			} else {
				pr = 0;
				while ((f[0] >= '0') && (f[0] <= '9')) {
					pr = pr * 10 + f[0] - '0';
					f++;
				}
			}
		}

		/* 处理整数大小覆盖设置 */
		switch (f[0]) {
			case 'h':
				fl |= _HALFWIDTH;
				++f;
				if (f[0] == 'h')
					++f;
				break;
			case 'l':
				fl |= ((sizeof(long) == 8) ? _INTMAX : 0);
				++f;
				if (f[0] == 'l') {
					fl |= _INTMAX;
					++f;
				}
				break;
			case 'j':
				fl |= (sizeof(size_t) == 8) ? _INTMAX : 0;
				++f;
				break;
			case 'z':
				fl |= (sizeof(ptrdiff_t) == 8) ? _INTMAX : 0;
				++f;
				break;
			case 't':
				fl |= (sizeof(ptrdiff_t) == 8) ? _INTMAX : 0;
				++f;
				break;
			case 'I':
				if ((f[1] == '6') && (f[2] == '4')) {
					fl |= _INTMAX;
					f += 3;
				} else if ((f[1] == '3') && (f[2] == '2')) {
					f += 3;
				} else {
					fl |= ((sizeof(void *) == 8) ? _INTMAX : 0);
					++f;
				}
				break;
			default:
				break;
		}

		/* 处理替换 */
		switch (f[0]) {
#define _NUMSZ 								512	/* 对于 E-308、E-307 足够大 */
			char num[_NUMSZ];
			char lead[8];
			char tail[8];
			char *s;
			char const *h;
			uint32_t l, n, cs;
			uint64_t n64;
			double fv;
			int32_t dp;
			char const *sn;

		case 's':
			/* 获取字符串 */
			s = va_arg(va, char *);
			if (s == 0)
				s = (char *)"null";
			/* 获取字符串长度，但不超过 ~0u */
			l = _strlen_limited(s,  (pr >= 0) ? (uint32_t) pr : ~0u);
			lead[0] = 0;
			tail[0] = 0;
			pr = 0;
			dp = 0;
			cs = 0;
			/* 复制字符串 */
			goto scopy;

		case 'c':
			/* 获取字符 */
			s = num + _NUMSZ - 1;
			*s = (char)va_arg(va, int32_t);
			l = 1;
			lead[0] = 0;
			tail[0] = 0;
			pr = 0;
			dp = 0;
			cs = 0;
			goto scopy;

		case 'n': {
			/* “%n”记录已经打印的字符的数量并将其赋值给相应的变量，而没有输出 */
			/* 例如，printf("Hello, %nWorld!", &c) */
			/* 此例将打印“Hello, World!”，并将 c 赋值为 6 */
			int32_t *d = va_arg(va, int32_t *);
			*d = tlen + (int32_t)(bf - buf);
		}
		break;

		case 'A':
		case 'a':
			/* 十六进制浮点数 */
			h = (f[0] == 'A') ? hexu : hex;
			fv = va_arg(va, double);
			if (pr == -1)
				pr = 6;
			/* 将 double 读取为字符串 */
			if (_real_to_parts((int64_t *)&n64, &dp, fv))
				fl |= _NEGATIVE;

			s = num + 64;

			_lead_sign(fl, lead);

			if (dp == -1023)
				dp = (n64) ? -1022 : 0;
			else
				n64 |= (((uint64_t)1) << 52);
			n64 <<= (64 - 56);
			if (pr < 15)
				n64 += ((((uint64_t)8) << 56) >> (pr * 4));
			/* 添加前导字符 */

			lead[1 + lead[0]] = '0';
			lead[2 + lead[0]] = 'x';
			lead[0] += 2;

			*s++ = h[(n64 >> 60) & 15];
			n64 <<= 4;
			if (pr)
				*s++ = _period;
			sn = s;

			/* 打印 bit */
			n = pr;
			if (n > 13)
				n = 13;
			if (pr > (int32_t)n)
				tz = pr - n;
			pr = 0;
			while (n--) {
				*s++ = h[(n64 >> 60) & 15];
				n64 <<= 4;
			}

			/* 打印 expo */
			tail[1] = h[17];
			if (dp < 0) {
				tail[2] = '-';
				dp = -dp;
			} else {
				tail[2] = '+';
			}
			n = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
			tail[0] = (char)n;
			for (;;) {
				tail[n] = '0' + dp % 10;
				if (n <= 3)
					break;
				--n;
				dp /= 10;
			}

			dp = (int32_t) (s - sn);
			l = (int32_t) (s - (num + 64));
			s = num + 64;
			cs = 1 + (3 << 24);
			goto scopy;

		case 'G':
		case 'g':
			/* 浮点数 */
			h = (f[0] == 'G') ? hexu : hex;
			fv = va_arg(va, double);
			if (pr == -1)
				pr = 6;
			else if (pr == 0)
				pr = 1;
			/* 将 double 读取为字符串 */
			if (_real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000))
				fl |= _NEGATIVE;

			/* 设置精度 */
			n = pr;
			if (l > (uint32_t )pr)
				l = pr;
			while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
				--pr;
				--l;
			}

			/* 是否使用 %e */
			if ((dp <= -4) || (dp > (int32_t)n)) {
				if (pr > (int32_t)l)
					pr = l - 1;
				else if (pr)
					--pr;
				goto doexpfromg;
			}
			if (dp > 0) {
				pr = (dp < (int32_t)l) ? l - dp : 0;
			} else {
				pr = -dp + ((pr > (int32_t)l) ? (int32_t)l : pr);
			}
			goto dofloatfromg;

		case 'E':
		case 'e':
			/* 浮点数 */
			h = (f[0] == 'E') ? hexu : hex;
			fv = va_arg(va, double);
			if (pr == -1)
				pr = 6;
			/* 将 double 读取为字符串 */
			if (_real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000))
				fl |= _NEGATIVE;
		doexpfromg:
			tail[0] = 0;
			_lead_sign(fl, lead);
			if (dp == _SPECIAL) {
				s = (char *)sn;
				cs = 0;
				pr = 0;
				goto scopy;
			}
			s = num + 64;
			/* 处理前导字符 */
			*s++ = sn[0];

			if (pr)
				*s++ = _period;

			/* 处理小数点后 */
			if ((l - 1) > (uint32_t)pr)
				l = pr + 1;
			for (n = 1; n < l; n++)
				*s++ = sn[n];
			/* 尾随零 */
			tz = pr - (l - 1);
			pr = 0;
			tail[1] = h[0xe];
			dp -= 1;
			if (dp < 0) {
				tail[2] = '-';
				dp = -dp;
			} else {
				tail[2] = '+';
			}

			n = (dp >= 100) ? 5 : 4;

			tail[0] = (char)n;
			for (;;) {
				tail[n] = '0' + dp % 10;
				if (n <= 3)
					break;
				--n;
				dp /= 10;
			}
			cs = 1 + (3 << 24);
			goto flt_lead;

		case 'f': 
			/* 浮点数 */
			fv = va_arg(va, double);
		doafloat:
			/* 处理千位 */
			if (fl & _METRIC_SUFFIX) {
				double divisor;
				divisor = 1000.0f;
				if (fl & _METRIC_1024)
					divisor = 1024.0;
				while (fl < 0x4000000) {
					if ((fv < divisor) && (fv > -divisor))
						break;
					fv /= divisor;
					fl += 0x1000000;
				}
			}
			if (pr == -1)
				pr = 6;
			/* 将 double 读取为字符串 */
			if (_real_to_str(&sn, &l, num, &dp, fv, pr))
				fl |= _NEGATIVE;
		dofloatfromg:
			tail[0] = 0;
			_lead_sign(fl, lead);
			if (dp == _SPECIAL) {
				s = (char *)sn;
				cs = 0;
				pr = 0;
				goto scopy;
			}
			s = num + 64;

			/* 处理小数样式 */
			if (dp <= 0) {
				int32_t i;
				/* 0.000*000xxxx */
				*s++ = '0';
				if (pr)
					*s++ = _period;
				n = -dp;
				if ((int32_t)n > pr)
					n = pr;
				i = n;
				while (i) {
					if ((((uintptr_t)s) & 3) == 0)
						break;
					*s++ = '0';
					--i;
				}
				while (i >= 4) {
					*(uint32_t *)s = 0x30303030;
					s += 4;
					i -= 4;
				}
				while (i) {
					*s++ = '0';
					--i;
				}
				if ((int32_t)(l + n) > pr)
					l = pr - n;
				i = l;
				while (i) {
					*s++ = *sn++;
					--i;
				}
				tz = pr - (n + l);
				cs = 1 + (3 << 24);
			} else {
				cs = (fl & _TRIPLET_COMMA) ? ((600 - (uint32_t)dp) % 3) : 0;
				if ((uint32_t)dp >= l) {
					/* xxxx000*000.0 */
					n = 0;
					for (;;) {
						if ((fl & _TRIPLET_COMMA) && (++cs == 4)) {
							cs = 0;
							*s++ = _comma;
						} else {
							*s++ = sn[n];
							++n;
							if (n >= l)
								break;
						}
					}
					if (n < (uint32_t)dp) {
						n = dp - n;
						if ((fl & _TRIPLET_COMMA) == 0) {
							while (n) {
								if ((((uintptr_t)s) & 3) == 0)
									break;
								*s++ = '0';
								--n;
							}
							while (n >= 4) {
								*(uint32_t *)s = 0x30303030;
								s += 4;
								n -= 4;
							}
						}
						while (n) {
							if ((fl & _TRIPLET_COMMA) && (++cs == 4)) {
								cs = 0;
								*s++ = _comma;
							} else {
								*s++ = '0';
								--n;
							}
						}
					}
					cs = (int32_t)(s - (num + 64)) + (3 << 24);
					if (pr)
					{
						*s++ = _period;
						tz = pr;
					}
				} else {
					/* xxxxx.xxxx000*000 */
					n = 0;
					for (;;) {
						if ((fl & _TRIPLET_COMMA) && (++cs == 4)) {
							cs = 0;
							*s++ = _comma;
						} else {
							*s++ = sn[n];
							++n;
							if (n >= (uint32_t)dp)
								break;
						}
					}
					cs = (int32_t)(s - (num + 64)) + (3 << 24);
					if (pr)
						*s++ = _period;
					if ((l - dp) > (uint32_t)pr)
						l = pr + dp;
					while (n < l) {
						*s++ = sn[n];
						++n;
					}
					tz = pr - (l - dp);
				}
			}
			pr = 0;

			/* 处理 k,m,g,t */
			if (fl & _METRIC_SUFFIX) {
				char idx;
				idx = 1;
				if (fl & _METRIC_NOSPACE)
					idx = 0;
				tail[0] = idx;
				tail[1] = ' ';
				{
					if (fl >> 24) {
						if (fl & _METRIC_1024)
							tail[idx + 1] = "_KMGT"[fl >> 24];
						else
							tail[idx + 1] = "_kMGT"[fl >> 24];
						idx++;
						if (fl & _METRIC_1024 && !(fl & _METRIC_JEDEC)) {
							tail[idx + 1] = 'i';
							idx++;
						}
						tail[0] = idx;
					}
				}
			};

		flt_lead:
			l = (uint32_t)(s - (num + 64));
			s = num + 64;
			goto scopy;

		case 'B':
		case 'b':
			/* 二进制 */
			h = (f[0] == 'B') ? hexu : hex;
			lead[0] = 0;
			if (fl & _LEADING_0X) {
				lead[0] = 2;
				lead[1] = '0';
				lead[2] = h[0xb];
			}
			l = (8 << 4) | (1 << 8);
			goto radixnum;

		case 'o': 
			/* 八进制 */
			h = hexu;
			lead[0] = 0;
			if (fl & _LEADING_0X) {
				lead[0] = 1;
				lead[1] = '0';
			}
			l = (3 << 4) | (3 << 8);
			goto radixnum;

		case 'p':
			/* 指针 */
			fl |= (sizeof(void *) == 8) ? _INTMAX : 0;
			pr = sizeof(void *) * 2;
			fl &= ~_LEADINGZERO;
			__attribute__((fallthrough));
		case 'X':
		case 'x':
			/* 十六进制 */
			h = (f[0] == 'X') ? hexu : hex;
			l = (4 << 4) | (4 << 8);
			lead[0] = 0;
			if (fl & _LEADING_0X) {
				lead[0] = 2;
				lead[1] = '0';
				lead[2] = h[16];
			}
		radixnum:
			if (fl & _INTMAX)
				n64 = va_arg(va, uint64_t);
			else
				n64 = va_arg(va, uint32_t);

			s = num + _NUMSZ;
			dp = 0;
			/* 清除尾随零和前导零 */
			tail[0] = 0;
			if (n64 == 0) {
				lead[0] = 0;
				if (pr == 0) {
					l = 0;
					cs = 0;
					goto scopy;
				}
			}
			/* 转换为字符串 */
			for (;;) {
				*--s = h[n64 & ((1 << (l >> 8)) - 1)];
				n64 >>= (l >> 8);
				if (!((n64) || ((int32_t)((num + _NUMSZ) - s) < pr)))
					break;
				if (fl & _TRIPLET_COMMA) {
					++l;
					if ((l & 15) == ((l >> 4) & 15)) {
						l &= ~15;
						*--s = _comma;
					}
				}
			};
			cs = (uint32_t)((num + _NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
			l = (uint32_t)((num + _NUMSZ) - s);	/* 获取复制的长度 */
			goto scopy;

		case 'u':
		case 'i':
		case 'd':
			/* 整数 */
			if (fl & _INTMAX) {
				int64_t i64 = va_arg(va, int64_t);
				n64 = (uint64_t)i64;
				if ((f[0] != 'u') && (i64 < 0)) {
					n64 = (uint64_t)-i64;
					fl |= _NEGATIVE;
				}
			} else {
				int32_t i = va_arg(va, int32_t);
				n64 = (uint32_t)i;
				if ((f[0] != 'u') && (i < 0)) {
					n64 = (uint32_t)-i;
					fl |= _NEGATIVE;
				}
			}

			if (fl & _METRIC_SUFFIX) {
				if (n64 < 1024)
					pr = 0;
				else if (pr == -1)
					pr = 1;
				fv = (double)(int64_t)n64;
				goto doafloat;
			}

			/* 转换为字符串 */
			s = num + _NUMSZ;
			l = 0;

			for (;;) {
				/* 以 32-bit 块进行 */
				char *o = s - 8;
				if (n64 >= 100000000) {
					n = (uint32_t)(n64 % 100000000);
					n64 /= 100000000;
				} else {
					n = (uint32_t)n64;
					n64 = 0;
				}
				if ((fl & _TRIPLET_COMMA) == 0) {
					do {
						s -= 2;
						*(uint16_t *)s = *(uint16_t *)&_digitpair.pair[(n % 100) * 2];
						n /= 100;
					} while (n);
				}
				while (n) {
					if ((fl & _TRIPLET_COMMA) && (l++ == 3)) {
						l = 0;
						*--s = _comma;
						--o;
					} else {
						*--s = (char)(n % 10) + '0';
						n /= 10;
					}
				}
				if (n64 == 0) {
					if ((s[0] == '0') && (s != (num + _NUMSZ)))
						++s;
					break;
				}
				while (s != o)
					if ((fl & _TRIPLET_COMMA) && (l++ == 3)) {
						l = 0;
						*--s = _comma;
						--o;
					} else {
						*--s = '0';
					}
			}

			tail[0] = 0;
			_lead_sign(fl, lead);

			/* 获取复制的长度 */
			l = (uint32_t)((num + _NUMSZ) - s);
			if (l == 0) {
				*--s = '0';
				l = 1;
			}
			cs = l + (3 << 24);
			if (pr < 0)
				pr = 0;

		scopy:
			if (pr < (int32_t)l)
				pr = l;
			n = pr + lead[0] + tail[0] + tz;
			if (fw < (int32_t)n)
				fw = n;
			fw -= n;
			pr -= l;

			/* 处理右对齐和前导零 */
			if ((fl & _LEFTJUST) == 0) {
				if (fl & _LEADINGZERO) {
					pr = (fw > pr) ? fw : pr;
					fw = 0;
				} else {
					fl &= ~_TRIPLET_COMMA;
				}
			}

			/* 复制空格和/或零 */
			if (fw + pr) {
				int32_t i;
				uint32_t c;

				/* 复制前导空格 */
				if ((fl & _LEFTJUST) == 0)
					while (fw > 0) {
						_cb_buf_clamp(i, fw);
						fw -= i;
						while (i) {
							if ((((uintptr_t)bf) & 3) == 0)
								break;
							*bf++ = ' ';
							--i;
						}
						while (i >= 4) {
							*(uint32_t *)bf = 0x20202020;
							bf += 4;
							i -= 4;
						}
						while (i) {
							*bf++ = ' ';
							--i;
						}
						_chk_cb_buf(1);
					}

				/* 复制前导符 */
				sn = lead + 1;
				while (lead[0]) {
					_cb_buf_clamp(i, lead[0]);
					lead[0] -= (char)i;
					while (i) {
						*bf++ = *sn++;
						--i;
					}
					_chk_cb_buf(1);
				}

				/* 复制前导零 */
				c = cs >> 24;
				cs &= 0xffffff;
				cs = (fl & _TRIPLET_COMMA) ? ((uint32_t)(c - ((pr + cs) % (c + 1)))) : 0;
				while (pr > 0) {
					_cb_buf_clamp(i, pr);
					pr -= i;
					if ((fl & _TRIPLET_COMMA) == 0) {
						while (i) {
							if ((((uintptr_t)bf) & 3) == 0)
								break;
							*bf++ = '0';
							--i;
						}
						while (i >= 4) {
							*(uint32_t *)bf = 0x30303030;
							bf += 4;
							i -= 4;
						}
					}
					while (i) {
						if ((fl & _TRIPLET_COMMA) && (cs++ == c)) {
							cs = 0;
							*bf++ = _comma;
						}
						else
							*bf++ = '0';
						--i;
					}
					_chk_cb_buf(1);
				}
			}

			/* 复制剩余的前导符 */
			sn = lead + 1;
			while (lead[0]) {
				int32_t i;
				_cb_buf_clamp(i, lead[0]);
				lead[0] -= (char)i;
				while (i) {
					*bf++ = *sn++;
					--i;
				}
				_chk_cb_buf(1);
			}

			/* 复制字符串 */
			n = l;
			while (n) {
				int32_t i;
				_cb_buf_clamp(i, n);
				n -= i;
				while (i >= 4) {
					*(uint32_t volatile *)bf = *(uint32_t volatile *)s;
					bf += 4;
					s += 4;
					i -= 4;
				}
				while (i) {
					*bf++ = *s++;
					--i;
				}
				_chk_cb_buf(1);
			}

			/* 复制尾随零 */
			while (tz) {
				int32_t i;
				_cb_buf_clamp(i, tz);
				tz -= i;
				while (i) {
					if ((((uintptr_t)bf) & 3) == 0)
						break;
					*bf++ = '0';
					--i;
				}
				while (i >= 4) {
					*(uint32_t *)bf = 0x30303030;
					bf += 4;
					i -= 4;
				}
				while (i) {
					*bf++ = '0';
					--i;
				}
				_chk_cb_buf(1);
			}

			/* 复制尾随符 */
			sn = tail + 1;
			while (tail[0]) {
				int32_t i;
				_cb_buf_clamp(i, tail[0]);
				tail[0] -= (char)i;
				while (i) {
					*bf++ = *sn++;
					--i;
				}
				_chk_cb_buf(1);
			}

			/* 处理左对齐 */
			if (fl & _LEFTJUST)
				if (fw > 0) {
					while (fw) {
						int32_t i;
						_cb_buf_clamp(i, fw);
						fw -= i;
						while (i) {
							if ((((uintptr_t)bf) & 3) == 0)
								break;
							*bf++ = ' ';
							--i;
						}
						while (i >= 4) {
							*(uint32_t *)bf = 0x20202020;
							bf += 4;
							i -= 4;
						}
						while (i--)
							*bf++ = ' ';
						_chk_cb_buf(1);
					}
				}
			break;

		default: /* 未知，仅复制 */
			s = num + _NUMSZ - 1;
			*s = f[0];
			l = 1;
			fw = fl = 0;
			lead[0] = 0;
			tail[0] = 0;
			pr = 0;
			dp = 0;
			cs = 0;
			goto scopy;
		}
		++f;
	}
endfmt:

	if (!callback)
		*bf = 0;
	else
		_flush_cb();

done:
	return tlen + (int32_t)(bf - buf);
}

/* 接口 */

int32_t sprintf(char *buf, char const *fmt, ...)
{
	int32_t result;
	va_list va;
	va_start(va, fmt);
	result = _vsprintfcb(0, 0, buf, fmt, va);
	va_end(va);
	return result;
}

typedef struct _context
{
	char *buf;
	int32_t count;
	int32_t length;
	char tmp[_SPRINTF_MIN];
} _context;

static char *_clamp_callback(const char *buf, void *user, int32_t len)
{
	_context *c = (_context *)user;
	c->length += len;

	if (len > c->count)
		len = c->count;

	if (len) {
		if (buf != c->buf) {
			const char *s, *se;
			char *d;
			d = c->buf;
			s = buf;
			se = buf + len;
			do {
				*d++ = *s++;
			} while (s < se);
		}
		c->buf += len;
		c->count -= len;
	}

	if (c->count <= 0)
		return c->tmp;
	return (c->count >= _SPRINTF_MIN) ? c->buf : c->tmp;
}

static char *_count_clamp_callback(const char *buf, void *user, int32_t len)
{
	_context *c = (_context *)user;
	(void)sizeof(buf);

	c->length += len;
	return c->tmp;
}

int32_t vsnprintf(char *buf, int32_t count, char const *fmt, va_list va)
{
	_context c;

	if ((count == 0) && !buf) {
		c.length = 0;

		_vsprintfcb(_count_clamp_callback, &c, c.tmp, fmt, va);
	} else {
		int32_t l;

		c.buf = buf;
		c.count = count;
		c.length = 0;

		_vsprintfcb(_clamp_callback, &c, _clamp_callback(0, &c, 0), fmt, va);

		l = (int32_t)(c.buf - buf);
		if (l >= count) /* 只能小于等于 */
			l = count - 1;
		buf[l] = 0;
	}

	return c.length;
}

int32_t snprintf(char *buf, int32_t count, char const *fmt, ...)
{
	int32_t result;
	va_list va;
	va_start(va, fmt);

	result = vsnprintf(buf, count, fmt, va);
	va_end(va);

	return result;
}

int32_t vsprintf(char *buf, char const *fmt, va_list va)
{
	return _vsprintfcb(0, 0, buf, fmt, va);
}

/* 浮点数处理函数 */

#define _COPYFP(dest, src) {							\
		int32_t cn;											\
		for (cn = 0; cn < 8; cn++)						\
			((char *)&dest)[cn] = ((char *)&src)[cn];	\
	}

/* 获取浮点数信息 */
static int32_t _real_to_parts(int64_t *bits, int32_t *expo, double value)
{
	double d;
	int64_t b = 0;

	d = value;

	_COPYFP(b, d);

	*bits = b & ((((uint64_t)1) << 52) - 1);
	*expo = (int32_t)(((b >> 52) & 2047) - 1023);

	return (int32_t)((uint64_t)b >> 63);
}

static double const _bot[23] = {
	1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
	1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022};
static double const _negbot[22] = {
	1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
	1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022};
static double const _negboterr[22] = {
	-5.551115123125783e-018, -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
	4.5251888174113739e-024, -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028, 2.0113352370744385e-029,
	-3.0373745563400371e-030, 1.1806906454401013e-032, -7.7705399876661076e-032, 2.0902213275965398e-033, -7.1542424054621921e-034, -7.1542424054621926e-035,
	2.4754073164739869e-036, 5.4846728545790429e-037, 9.2462547772103625e-038, -4.8596774326570872e-039};
static double const _top[13] = {
	1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299};
static double const _negtop[13] = {
	1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299};
static double const _toperr[13] = {
	8388608,
	6.8601809640529717e+028,
	-7.253143638152921e+052,
	-4.3377296974619174e+075,
	-1.5559416129466825e+098,
	-3.2841562489204913e+121,
	-3.7745893248228135e+144,
	-1.7356668416969134e+167,
	-3.8893577551088374e+190,
	-9.9566444326005119e+213,
	6.3641293062232429e+236,
	-5.2069140800249813e+259,
	-5.2504760255204387e+282};
static double const _negtoperr[13] = {
	3.9565301985100693e-040, -2.299904345391321e-063, 3.6506201437945798e-086, 1.1875228833981544e-109,
	-5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178, -5.7778912386589953e-201,
	7.4997100559334532e-224, -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
	8.0970921678014997e-317};

static uint64_t const _powten[20] = {
	1,
	10,
	100,
	1000,
	10000,
	100000,
	1000000,
	10000000,
	100000000,
	1000000000,
	10000000000ULL,
	100000000000ULL,
	1000000000000ULL,
	10000000000000ULL,
	100000000000000ULL,
	1000000000000000ULL,
	10000000000000000ULL,
	100000000000000000ULL,
	1000000000000000000ULL,
	10000000000000000000ULL};
#define _tento19th (1000000000000000000ULL)

#define _ddmulthi(oh, ol, xh, yh) {										\
		double ahi = 0, alo, bhi = 0, blo;								\
		int64_t bt;														\
		oh = xh * yh;													\
		_COPYFP(bt, xh);												\
		bt &= ((~(uint64_t)0) << 27);									\
		_COPYFP(ahi, bt);												\
		alo = xh - ahi;													\
		_COPYFP(bt, yh);												\
		bt &= ((~(uint64_t)0) << 27);									\
		_COPYFP(bhi, bt);												\
		blo = yh - bhi;													\
		ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo;	\
	}

#define _ddtoS64(ob, xh, xl) {				\
		double ahi = 0, alo, vh, t;			\
		ob = (int64_t)xh;					\
		vh = (double)ob;					\
		ahi = (xh - vh);					\
		t = (ahi - xh);						\
		alo = (xh - (ahi - t)) - (vh + t);	\
		ob += (int64_t)(ahi + alo + xl);	\
	}

#define _ddrenorm(oh, ol) {		\
		double s;				\
		s = oh + ol;			\
		ol = ol - (s - oh);		\
		oh = s;					\
	}

#define _ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define _ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

static void _raise_to_power10(double *ohi, double *olo, double d, int32_t power)
{
	double ph, pl;
	if ((power >= 0) && (power <= 22)) {
		_ddmulthi(ph, pl, d, _bot[power]);
	} else {
		int32_t e, et, eb;
		double p2h, p2l;

		e = power;
		if (power < 0)
			e = -e;
		et = (e * 0x2c9) >> 14; /* %23 */
		if (et > 13)
			et = 13;
		eb = e - (et * 23);

		ph = d;
		pl = 0.0;
		if (power < 0) {
			if (eb) {
				--eb;
				_ddmulthi(ph, pl, d, _negbot[eb]);
				_ddmultlos(ph, pl, d, _negboterr[eb]);
			}
			if (et) {
				_ddrenorm(ph, pl);
				--et;
				_ddmulthi(p2h, p2l, ph, _negtop[et]);
				_ddmultlo(p2h, p2l, ph, pl, _negtop[et], _negtoperr[et]);
				ph = p2h;
				pl = p2l;
			}
		} else {
			if (eb) {
				e = eb;
				if (eb > 22)
					eb = 22;
				e -= eb;
				_ddmulthi(ph, pl, d, _bot[eb]);
				if (e) {
					_ddrenorm(ph, pl);
					_ddmulthi(p2h, p2l, ph, _bot[e]);
					_ddmultlos(p2h, p2l, _bot[e], pl);
					ph = p2h;
					pl = p2l;
				}
			}
			if (et) {
				_ddrenorm(ph, pl);
				--et;
				_ddmulthi(p2h, p2l, ph, _top[et]);
				_ddmultlo(p2h, p2l, ph, pl, _top[et], _toperr[et]);
				ph = p2h;
				pl = p2l;
			}
		}
	}
	_ddrenorm(ph, pl);
	*ohi = ph;
	*olo = pl;
}

/*
	给定一个浮点数，返回：
		bits：有效位
		decimal_pos：小数点的位置
	+/-infinity 和 NaN 通过 decimal_pos 参数返回的特殊值来标识
*/
static int32_t _real_to_str(char const **start, uint32_t *len, char *out, int32_t *decimal_pos, double value, uint32_t frac_digits)
{
	double d;
	int64_t bits = 0;
	int32_t expo, e, ng, tens;

	d = value;
	_COPYFP(bits, d);
	expo = (int32_t)((bits >> 52) & 2047);
	ng = (int32_t)((uint64_t)bits >> 63);
	if (ng)
		d = -d;

	if (expo == 2047) {
		*start = (bits & ((((uint64_t)1) << 52) - 1)) ? "NaN" : "Inf";
		*decimal_pos = _SPECIAL;
		*len = 3;
		return ng;
	}

	if (expo == 0) {
		if (((uint64_t)bits << 1) == 0) {
			*decimal_pos = 1;
			*start = out;
			out[0] = '0';
			*len = 1;
			return ng;
		}
		{
			int64_t v = ((uint64_t)1) << 51;
			while ((bits & v) == 0) {
				--expo;
				v >>= 1;
			}
		}
	}

	/* 找到小数指数以及值的小数位 */
	{
		double ph, pl;

		tens = expo - 1023;
		tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

		_raise_to_power10(&ph, &pl, d, 18 - tens);

		_ddtoS64(bits, ph, pl);

		if (((uint64_t)bits) >= _tento19th)
			++tens;
	}

	/* 在整数内舍入 */
	frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
	if ((frac_digits < 24)) {
		uint32_t dg = 1;
		if ((uint64_t)bits >= _powten[9])
			dg = 10;
		while ((uint64_t)bits >= _powten[dg]) {
			++dg;
			if (dg == 20)
				goto noround;
		}
		if (frac_digits < dg) {
			uint64_t r;
			/* 向上取整*/
			e = dg - frac_digits;
			if ((uint32_t)e >= 24)
				goto noround;
			r = _powten[e];
			bits = bits + (r / 2);
			if ((uint64_t)bits >= _powten[dg])
				++tens;
			bits /= r;
		}
	noround:;
	}

	/* 删除尾随零 */
	if (bits) {
		uint32_t n;
		for (;;) {
			if (bits <= 0xffffffff)
				break;
			if (bits % 1000)
				goto donez;
			bits /= 1000;
		}
		n = (uint32_t)bits;
		while ((n % 1000) == 0)
			n /= 1000;
		bits = n;
	donez:;
	}

	/* 转换为字符串 */
	out += 64;
	e = 0;
	for (;;) {
		uint32_t n;
		char *o = out - 8;
		/* 以 32-bit 块进行 */
		if (bits >= 100000000) {
			n = (uint32_t)(bits % 100000000);
			bits /= 100000000;
		} else {
			n = (uint32_t)bits;
			bits = 0;
		}
		while (n) {
			out -= 2;
			*(uint16_t *)out = *(uint16_t *)&_digitpair.pair[(n % 100) * 2];
			n /= 100;
			e += 2;
		}
		if (bits == 0) {
			if ((e) && (out[0] == '0')) {
				++out;
				--e;
			}
			break;
		}
		while (out != o) {
			*--out = '0';
			++e;
		}
	}

	*decimal_pos = tens;
	*start = out;
	*len = e;
	return ng;
}
