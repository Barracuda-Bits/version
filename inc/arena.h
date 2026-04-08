#ifndef __ARENA_H__
#define __ARENA_H__

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "vmem.h"

typedef struct arena arena_t;
typedef struct transient_marker transient_marker_t;

arena_t* arena_create(size_t size);
anyptr_t arena_alloc(arena_t* arena, size_t size, size_t align);
anyptr_t arena_alloc_zero(arena_t* arena, size_t size, size_t alignment);
void arena_destroy(arena_t* arena);

void arena_begin_scope(arena_t* arena, transient_marker_t* out_marker);
void arena_end_scope(arena_t* arena, transient_marker_t marker);

#endif // __ARENA_H__

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
