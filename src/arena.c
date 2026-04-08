#ifndef __ARENA_H__
#define __ARENA_H__

// EXTERNAL INCLUDES
#ifdef _WIN32
#	include <Windows.h>
#endif
// INTERNAL INCLUDES
#include "arena.h"
#include "vmem.h"

// ***************************************************************
typedef struct arena
{
	size_t offset;
	size_t reserved;
	size_t committed;
	// data follows
} arena_t;
typedef struct transient_marker
{
	size_t offset;
} transient_marker_t;
#define ARENA_DATA(a) ((arena_t*)(a) + 1)
#define ARENA_COMMIT_GRANULARITY (KiB(64))
// ***************************************************************
arena_t* arena_create(size_t size)
{
	arena_t* arena = 0;

	if (size == 0)
	{
		return NULL;
	}
	
	size_t reserve_size = align_forward(sizeof(arena_t) + size, ARENA_COMMIT_GRANULARITY);
	size_t commit_size = align_forward(sizeof(arena_t), ARENA_COMMIT_GRANULARITY);

#ifdef _WIN32
	arena = (arena_t*)VirtualAlloc(
		NULL,
		reserve_size,
		MEM_RESERVE,
		PAGE_READWRITE
	);

	arena = (arena_t*)VirtualAlloc(
		arena,
		commit_size,
		MEM_COMMIT,
		PAGE_READWRITE
	);
#endif

	arena->committed = commit_size;
	arena->offset = 0;
	arena->reserved = reserve_size;

	return arena;
}
// ***************************************************************
anyptr_t arena_alloc(arena_t* arena, size_t size, size_t align)
{
	if (!arena) return NULL;

	byte* data = (byte*)ARENA_DATA(arena);

	if (0 == align)
	{
		align = sizeof(anyptr_t);
	}

	anyptr_t base = (anyptr_t)data;
	anyptr_t current = (anyptr_t)((size_t)base + arena->offset);

	size_t aligned_ptr = align_forward((size_t)current, align);

	size_t aligned_offset = aligned_ptr - (size_t)base;

	// [pkrs] overflow protection (should never happen)
	if (aligned_offset > SIZE_MAX - size)
	{
		return NULL;
	}

	size_t new_offset = aligned_offset + size;
	size_t required_commit = sizeof(arena_t) + new_offset;

	if (required_commit > arena->committed)
	{
		size_t new_commit = align_forward(
			required_commit,
			ARENA_COMMIT_GRANULARITY
		);

		if (new_commit > arena->reserved)
		{
			return NULL;
		}

		size_t grow = new_commit - arena->committed;

		if (0 < grow)
		{
			void* commit_ptr =
				(byte*)arena + arena->committed;
#ifdef _WIN32
			if (!VirtualAlloc(
				commit_ptr,
				grow,
				MEM_COMMIT,
				PAGE_READWRITE))
			{
				return NULL;
			}
#endif

			arena->committed = new_commit;
		}
	}

	arena->offset = new_offset;

	return (void*)aligned_ptr;
}
void arena_destroy(arena_t* arena)
{
	if (!arena) return;

#ifdef _WIN32
	VirtualFree(arena, 0, MEM_RELEASE);
#endif
}
// ***************************************************************
anyptr_t arena_alloc_zero(arena_t* arena, size_t size, size_t alignment)
{
	anyptr_t ptr = arena_alloc(arena, size, alignment);
	MEMORY_ZERO(ptr, size);
	return ptr;
}
// ***************************************************************
void arena_begin_scope(arena_t* arena, transient_marker_t* out_marker)
{
	if (!arena || !out_marker) return;
	out_marker->offset = arena->offset;
}
// ***************************************************************
void arena_end_scope(arena_t* arena, transient_marker_t marker)
{
	if (!arena) return;
	arena->offset = marker.offset;
}
// ***************************************************************

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
