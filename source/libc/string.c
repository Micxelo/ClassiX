/*
	libc/string.c
*/

#include <stddef.h>
#include <string.h>

inline char *strcpy(char *dest, const char *src)
{
	asm("cld                 \n"
		"1:  lodsb           \n"
		"    stosb           \n"
		"    testb %%al, %%al\n"
		"    jne 1b            "
		::"S"(src), "D"(dest)
		:"ax");
	return dest;
}

inline char *strncpy(char *dest, const char *src, size_t count)
{
	asm("cld\n"
		"1:  decl %2         \n"
		"    js 2f           \n"
		"    lodsb           \n"
		"    stosb           \n"
		"    testb %%al, %%al\n"
		"    jne 1b          \n"
		"    rep             \n"
		"    stosb           \n"
		"2:                    "
		::"S"(src), "D"(dest), "c"(count)
		:"ax");
	return dest;
}

inline char *strcat(char *dest, const char *src)
{
	asm("    cld             \n"
		"    repne           \n"
		"    scasb           \n"
		"    decl %1         \n"
		"1:  lodsb           \n"
		"    stosb           \n"
		"    testb %%al, %%al\n"
		"    jne 1b            "
		::"S" (src), "D" (dest), "a" (0), "c" (0xffffffff));
	return dest;
}

inline char *strncat(char *dest, const char *src, size_t count)
{
	asm("    cld             \n"
		"    repne           \n"
		"    scasb           \n"
		"    decl %1         \n"
		"    movl %4, %3     \n"
		"1:  decl %3         \n"
		"    js 2f           \n"
		"    lodsb           \n"
		"    stosb           \n"
		"    testb %%al, %%al\n"
		"    jne 1b          \n"
		"2:  xorl %2, %2     \n"
		"    stosb             "
		::"S"(src), "D"(dest), "a"(0), "c"(0xffffffff), "g"(count));
	return dest;
}

inline int strcmp(const char *s1, const char *s2)
{
	register int _res asm("ax");
	asm("    cld              \n"
		"1:  lodsb            \n"
		"    scasb            \n"
		"    jne 2f           \n"
		"    testb %%al, %%al \n"
		"    jne 1b           \n"
		"    xorl %%eax, %%eax\n"
		"    jmp 3f           \n"
		"2:  movl $1, %%eax   \n"
		"    jl 3f            \n"
		"    negl %%eax       \n"
		"3:                     "
		:"=a"(_res)
		:"D"(s1), "S"(s2));
	return _res;
}

inline int strncmp(const char *s1, const char *s2, size_t count)
{
	register int _res asm("ax");
	asm("    cld              \n"
		"1:  decl %3          \n"
		"    js 2f            \n"
		"    lodsb            \n"
		"    scasb            \n"
		"    jne 3f           \n"
		"    testb %%al, %%al \n"
		"    jne 1b           \n"
		"2:  xorl %%eax, %%eax\n"
		"    jmp 4f           \n"
		"3:  movl $1, %%eax   \n"
		"    jl 4f            \n"
		"    negl %%eax       \n"
		"4:                     "
		:"=a"(_res)
		:"D"(s1), "S"(s2), "c"(count));
	return _res;
}

inline char *strchr(const char *s, char c)
{
	register char *_res asm("ax");
	asm("    cld             \n"
		"    movb %%al, %%ah \n"
		"1:  lodsb           \n"
		"    cmpb %%ah, %%al \n"
		"    je 2f           \n"
		"    testb %%al, %%al\n"
		"    jne 1b          \n"
		"    movl $1, %1     \n"
		"2:  movl %1 ,%0     \n"
		"    decl %0           "
		:"=a"(_res)
		:"S"(s), "0"(c));
	return _res;
}

inline char *strrchr(const char *s, char c)
{
	register char *_res asm("dx");
	asm("    cld             \n"
		"    movb %%al, %%ah \n"
		"1:  lodsb           \n"
		"    cmpb %%ah, %%al \n"
		"    jne 2f          \n"
		"    movl %%esi, %0  \n"
		"    decl %0         \n"
		"2:  testb %%al, %%al\n"
		"    jne 1b            "
		:"=d"(_res)
		:"0"(0), "S"(s), "a"(c));
	return _res;
}

inline size_t strspn(const char *s, const char *accept)
{
	register char *_res asm("si");
	asm("    cld              \n"
		"    movl %4, %%edi   \n"
		"    repne            \n"
		"    scasb            \n"
		"    notl %%ecx       \n"
		"    decl %%ecx       \n"
		"    movl %%ecx, %%edx\n"
		"1:  lodsb            \n"
		"    testb %%al, %%al \n"
		"    je 2f            \n"
		"    movl %4, %%edi   \n"
		"    movl %%edx, %%ecx\n"
		"    repne            \n"
		"    scasb            \n"
		"    je 1b            \n"
		"2:  decl %0            "
		:"=S"(_res)
		:"a"(0), "c"(0xffffffff), "0"(s), "g"(accept)
		:"dx", "di");
	return (size_t) ( _res - s);
}

inline size_t strcspn(const char *s, const char *accept)
{
	register char *_res asm("si");
	asm("    cld              \n"
		"    movl %4, %%edi   \n"
		"    repne            \n"
		"    scasb            \n"
		"    notl %%ecx       \n"
		"    decl %%ecx       \n"
		"    movl %%ecx, %%edx\n"
		"1:  lodsb            \n"
		"    testb %%al, %%al \n"
		"    je 2f            \n"
		"    movl %4, %%edi   \n"
		"    movl %%edx, %%ecx\n"
		"    repne            \n"
		"    scasb            \n"
		"    jne 1b           \n"
		"2:  decl %0            "
		:"=S"(_res)
		:"a"(0), "c"(0xffffffff), "0"(s), "g"(accept)
		:"dx", "di");
	return (size_t) (_res - s);
}

inline char *strpbrk(const char *s, const char *accept)
{
	register char *_res asm("si");
	asm("    cld              \n"
		"    movl %4, %%edi   \n"
		"    repne            \n"
		"    scasb            \n"
		"    notl %%ecx       \n"
		"    decl %%ecx       \n"
		"    movl %%ecx, %%edx\n"
		"1:  lodsb            \n"
		"    testb %%al, %%al \n"
		"    je 2f            \n"
		"    movl %4, %%edi   \n"
		"    movl %%edx, %%ecx\n"
		"    repne            \n"
		"    scasb            \n"
		"    jne 1b           \n"
		"    decl %0          \n"
		"    jmp 3f           \n"
		"2:  xorl %0, %0      \n"
		"3:                     "
	:"=S"(_res)
	:"a"(0), "c"(0xffffffff), "0"(s), "g"(accept)
	:"dx", "di");
	return _res;
}

inline char *strstr(const char *s1, const char *s2)
{
	register char *_res asm("ax");
	asm("    cld               \n"
		"    movl %4, %%edi    \n"
		"    repne             \n"
		"    scasb             \n"
		"    notl %%ecx        \n"
		"    decl %%ecx        \n"
		"    movl %%ecx, %%edx \n"
		"1:  movl %4, %%edi    \n"
		"    movl %%esi, %%eax \n"
		"    movl %%edx, %%ecx \n"
		"    repe              \n"
		"    cmpsb             \n"
		"    je 2f             \n"
		"    xchgl %%eax, %%esi\n"
		"    incl %%esi        \n"
		"    cmpb $0, -1(%%eax)\n"
		"    jne 1b            \n"
		"    xorl %%eax, %%eax \n"
		"2:                      "
		:"=a"(_res)
		:"0"(0), "c"(0xffffffff), "S"(s1), "g"(s2)
		:"dx", "di");
	return _res;
}

inline size_t strlen(const char *s)
{
	register int _res asm("cx");
	asm("cld    \n"
		"repne  \n"
		"scasb  \n"
		"notl %0\n"
		"decl %0"
		:"=c"(_res)
		:"D"(s), "a"(0), "0"(0xffffffff));
	return _res;
}

static char *_strtok;

inline char *strtok(char *s, const char *delim)
{
	register char *_res asm("si");
	asm("    testl %1, %1     \n"
		"    jne 1f           \n"
		"    testl %0, %0     \n"
		"    je 7f            \n"
		"    movl %0, %1      \n"
		"1:  xorl %0, %0      \n"
		"    movl $-1, %%ecx  \n"
		"    xorl %%eax, %%eax\n"
		"    cld              \n"
		"    movl %4, %%edi   \n"
		"    repne            \n"
		"    scasb            \n"
		"    notl %%ecx       \n"
		"    decl %%ecx       \n"
		"    je 6f            \n"
		"    movl %%ecx, %%edx\n"
		"2:  lodsb            \n"
		"    testb %%al, %%al \n"
		"    je 6f            \n"
		"    movl %4, %%edi   \n"
		"    movl %%edx, %%ecx\n"
		"    repne            \n"
		"    scasb            \n"
		"    je 2b            \n"
		"    decl %1          \n"
		"    cmpb $0, (%1)    \n"
		"    je 6f            \n"
		"    movl %1, %0      \n"
		"3:  lodsb            \n"
		"    testb %%al, %%al \n"
		"    je 4f            \n"
		"    movl %4, %%edi   \n"
		"    movl %%edx, %%ecx\n"
		"    repne            \n"
		"    scasb            \n"
		"    jne 3b           \n"
		"    decl %1          \n"
		"    cmpb $0, (%1)    \n"
		"    je 4f            \n"
		"    movb $0, (%1)    \n"
		"    incl %1          \n"
		"    jmp 5f           \n"
		"4:  xorl %1, %1      \n"
		"5:  cmpb $0, (%0)    \n"
		"    jne 6f           \n"
		"    xorl %0, %0      \n"
		"6:  testl %0, %0     \n"
		"    jne 7f           \n"
		"    movl %0, %1      \n"
		"7:                     "
		:"=b"(_res), "=S"(_strtok)
		:"0"(_strtok), "1"(s), "g"(delim)
		:"ax", "cx", "dx", "di");
	return _res;
}

inline void *memcpy(void *dest, void *src, size_t n)
{
	asm("cld\n"
		"rep\n"
		"movsb"
		::"c"(n), "S"(src), "D" (dest));
	return dest;
}

inline void *memmove(void *dest, void *src, size_t n)
{
	if (dest < src) {
		asm("cld\n"
			"rep\n"
			"movsb"
			::"c"(n), "S"(src), "D"(dest));
	} else {
		asm("std\n"
			"rep\n"
			"movsb"
			::"c"(n), "S"(src + n - 1), "D"(dest + n - 1));
	}
	return dest;
}

inline int memcmp(void *s1, void *s2, size_t n)
{
	register int _res asm("ax");
	asm("    cld           \n"
		"    repe          \n"
		"    cmpsb         \n"
		"    je 1f         \n"
		"    movl $1, %%eax\n"
		"    jl 1f         \n"
		"    negl %%eax    \n"
		"1:                  "
		:"=a"(_res)
		:"0"(0), "D"(s1), "S"(s2), "c"(n));
	return _res;
}

inline void *memchr(void *s, unsigned char c, size_t n)
{
	if (!n) {
		return NULL;
	}

	register void *_res asm("di");
	asm("    cld        \n"
		"    repne      \n"
		"    scasb      \n"
		"    je 1f      \n"
		"    movl $1, %0\n"
		"1:  decl %0      "
		:"=D"(_res)
		:"a"(c), "D"(s), "c"(n));
	return _res;
}

inline void *memset(void *s, unsigned char c, size_t n)
{
	asm("cld\n"
		"rep\n"
		"stosb"
		::"a"(c), "D"(s), "c"(n));
	return s;
}
