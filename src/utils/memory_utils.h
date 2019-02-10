#pragma once

#include "utils/fixed_int.h"
#include "utils/assert.h"

#define ALIGN(ptr, alignment) (uint8*)(ptr) + (uint8)(((~(uintptr)(ptr)) + 1) & (uint8)(alignment - 1))
#define ALIGNOFFSET(ptr, alignment) (uint8)(ALIGN((ptr), (alignment)) - (uint8*)(ptr))

typedef uint64 memory_index;

typedef struct memory_arena
{
	void* memory;
	uint8* push_ptr;
	uint64 remaining_space;
} memory_arena;

// TODO(soimn): change to atomic operations when implementing the job system
inline void*
PushSize_(memory_arena* arena, memory_index size, uint8 alignment)
{
	void* result = {};
	uint8 adjustment = ALIGNOFFSET(arena->push_ptr, alignment);
	memory_index total_size = size + adjustment;

	if (total_size < arena->remaining_space)
	{
		result = ALIGN(arena->push_ptr, alignment);
		arena->push_ptr += total_size;
		arena->remaining_space -= total_size;
	}

	// NOTE(soimn): this is temporary
	Assert(result);

	return result;
}

inline void*
PushArray_(memory_arena* arena, memory_index size, uint8 alignment, memory_index count)
{
	void* result = {};
	uint8 adjustment = ALIGNOFFSET(arena->push_ptr + size, alignment);
	uint8 padding = ALIGNOFFSET(ALIGN(arena->push_ptr, alignment) + size, alignment);
	memory_index total_size = adjustment + count * (size + padding);

	if (total_size < arena->remaining_space)
	{
		result = ALIGN(arena->push_ptr, alignment);
		arena->push_ptr += total_size;
		arena->remaining_space -= total_size;
	}

	// NOTE(soimn): this is temporary
	Assert(result);

	return result;
}

// TODO(soimn): implement free list like functionality to enable freeing objects

#define PushStruct(arena, type) (type*) PushSize_(arena, sizeof(type), alignof(type))
#define PushArray(arena, type, count) (type*) PushArray_(arena, sizeof(type), alignof(type), count) 
#define PushSize(arena, size, alignment) PushSize_(arena, (size), (alignment))
