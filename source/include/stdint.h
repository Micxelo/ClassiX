/*
	include/stdint.h
*/

#ifndef _STDINT_H_
#define _STDINT_H_

#ifdef __cplusplus
	extern "C" {
#endif

#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ int8_t;
#endif
#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16_t;
#endif
#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32_t;
#endif
#ifdef __INT64_TYPE__
typedef __INT64_TYPE__ int64_t;
#endif
#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8_t;
#endif
#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16_t;
#endif
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
#endif
#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ uint64_t;
#endif

typedef __INT_LEAST8_TYPE__ int_least8_t;
typedef __INT_LEAST16_TYPE__ int_least16_t;
typedef __INT_LEAST32_TYPE__ int_least32_t;
typedef __INT_LEAST64_TYPE__ int_least64_t;
typedef __UINT_LEAST8_TYPE__ uint_least8_t;
typedef __UINT_LEAST16_TYPE__ uint_least16_t;
typedef __UINT_LEAST32_TYPE__ uint_least32_t;
typedef __UINT_LEAST64_TYPE__ uint_least64_t;


typedef __INT_FAST8_TYPE__ int_fast8_t;
typedef __INT_FAST16_TYPE__ int_fast16_t;
typedef __INT_FAST32_TYPE__ int_fast32_t;
typedef __INT_FAST64_TYPE__ int_fast64_t;
typedef __UINT_FAST8_TYPE__ uint_fast8_t;
typedef __UINT_FAST16_TYPE__ uint_fast16_t;
typedef __UINT_FAST32_TYPE__ uint_fast32_t;
typedef __UINT_FAST64_TYPE__ uint_fast64_t;


#ifdef __INTPTR_TYPE__
typedef __INTPTR_TYPE__ intptr_t;
#endif
#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__ uintptr_t;
#endif

#ifdef __PTRDIFF_TYPE__
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#endif

typedef __INTMAX_TYPE__ intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

#if __WORDSIZE == 64
#define __INT64_C(c)	c ## L
#define __UINT64_C(c)	c ## UL
#else
#define __INT64_C(c)	c ## LL
#define __UINT64_C(c)	c ## ULL
#endif


#define INT8_MIN			(-128)
#define INT16_MIN			(-32767-1)
#define INT32_MIN			(-2147483647-1)
#define INT64_MIN			(-__INT64_C(9223372036854775807)-1)

#define INT8_MAX			(127)
#define INT16_MAX			(32767)
#define INT32_MAX			(2147483647)
#define INT64_MAX			(__INT64_C(9223372036854775807))

#define UINT8_MAX			(255)
#define UINT16_MAX			(65535)
#define UINT32_MAX			(4294967295U)
#define UINT64_MAX			(__UINT64_C(18446744073709551615))

#define INT_LEAST8_MIN		(-128)
#define INT_LEAST16_MIN		(-32767-1)
#define INT_LEAST32_MIN		(-2147483647-1)
#define INT_LEAST64_MIN		(-__INT64_C(9223372036854775807)-1)

#define INT_LEAST8_MAX		(127)
#define INT_LEAST16_MAX		(32767)
#define INT_LEAST32_MAX		(2147483647)
#define INT_LEAST64_MAX		(__INT64_C(9223372036854775807))

#define UINT_LEAST8_MAX		(255)
#define UINT_LEAST16_MAX	(65535)
#define UINT_LEAST32_MAX	(4294967295U)
#define UINT_LEAST64_MAX	(__UINT64_C(18446744073709551615))

#define INT_FAST8_MIN		(-128)
#if __WORDSIZE == 64
#define INT_FAST16_MIN		(-9223372036854775807L-1)
#define INT_FAST32_MIN		(-9223372036854775807L-1)
#else
#define INT_FAST16_MIN		(-2147483647-1)
#define INT_FAST32_MIN		(-2147483647-1)
#endif
#define INT_FAST64_MIN		(-__INT64_C(9223372036854775807)-1)

#define INT_FAST8_MAX		(127)
#if __WORDSIZE == 64
#define INT_FAST16_MAX		(9223372036854775807L)
#define INT_FAST32_MAX		(9223372036854775807L)
#else
#define INT_FAST16_MAX		(2147483647)
#define INT_FAST32_MAX		(2147483647)
#endif
#define INT_FAST64_MAX		(__INT64_C(9223372036854775807))

#define UINT_FAST8_MAX		(255)
#if __WORDSIZE == 64
#define UINT_FAST16_MAX		(18446744073709551615UL)
#define UINT_FAST32_MAX		(18446744073709551615UL)
#else
#define UINT_FAST16_MAX		(4294967295U)
#define UINT_FAST32_MAX		(4294967295U)
#endif
#define UINT_FAST64_MAX		(__UINT64_C(18446744073709551615))

#if __WORDSIZE == 64
#define INTPTR_MIN			(-9223372036854775807L-1)
#define INTPTR_MAX			(9223372036854775807L)
#define UINTPTR_MAX			(18446744073709551615UL)
#else
#define INTPTR_MIN			(-2147483647-1)
#define INTPTR_MAX			(2147483647)
#define UINTPTR_MAX			(4294967295U)
#endif

#define INTMAX_MIN			(-__INT64_C(9223372036854775807) - 1)
#define INTMAX_MAX			(__INT64_C(9223372036854775807))
#define UINTMAX_MAX			(__UINT64_C(18446744073709551615))

#if __WORDSIZE == 64
#define PTRDIFF_MIN			(-9223372036854775807L - 1)
#define PTRDIFF_MAX			(9223372036854775807L)
#else
#if __WORDSIZE32_PTRDIFF_LONG
#define PTRDIFF_MIN			(-2147483647L-1)
#define PTRDIFF_MAX			(2147483647L)
#else
#define PTRDIFF_MIN			(-2147483647-1)
#define PTRDIFF_MAX			(2147483647)
#endif
#endif

#if __WORDSIZE == 64
#define SIZE_MAX			(18446744073709551615UL)
#else
#if __WORDSIZE32_SIZE_ULONG
#define SIZE_MAX			(4294967295UL)
#else
#define SIZE_MAX			(4294967295U)
#endif
#endif

#ifdef __cplusplus
	}
#endif

#endif
