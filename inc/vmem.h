#ifndef __VMEM_H__
#define __VMEM_H__

// EXTERNAL INCLUDES
#include <stddef.h>
// INTERNAL INCLUDES

typedef void* anyptr_t;
typedef unsigned char byte;

#ifdef _WIN32
#   include <direct.h>
#   define popen _popen
#   define pclose _pclose
#   define chdir _chdir
#   define PATH_SEP '\\'
#else
#   include <unistd.h>
#   define PATH_SEP '/'
#endif

#ifndef __cplusplus
typedef _Bool bool;
#	define true 1
#	define false 0
#	define alignof(type) _Alignof(type)
#endif

inline size_t KiB(size_t x) { return x * 1024; }
inline size_t MiB(size_t x) { return x * 1024 * 1024; }
inline size_t GiB(size_t x) { return x * 1024 * 1024 * 1024; }

inline size_t KB(size_t x) { return x * 1000; }
inline size_t MB(size_t x) { return x * 1000 * 1000; }
inline size_t GB(size_t x) { return x * 1000 * 1000 * 1000; }

inline size_t align_forward(size_t addr, size_t align)
{
	return (addr + (align - 1)) & ~(align - 1);
}

#define MEMORY_ZERO(ptr, size) { if (ptr) memset((ptr), 0, (size)); } while(0)

#endif // __VMEM_H__

/*
 * Copyright 2025 Barracuda Bits
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this work and associated documentation files (the "Work"), to deal in the
 * Work without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Work,
 * and to permit persons to whom the Work is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Work.
 *
 * THE WORK IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF, OR IN
 * CONNECTION WITH THE WORK OR THE USE OR OTHER DEALINGS IN THE WORK.
 */
