#pragma once

#include "utils/utility_defines.h"
#include "ant_types.h"
#include "utils/assert.h"

#define MEMORY_8BIT_ALIGNED  1
#define MEMORY_16BIT_ALIGNED 2
#define MEMORY_32BIT_ALIGNED 4
#define MEMORY_64BIT_ALIGNED 8

typedef struct memory_block
{
	memory_block* next;
	memory_block* previous;

	void* memory;
	memory_index remaining_space;
	uint8* push_ptr;
} memory_block;

#define MEMORY_ALLOCATE_MEMORY_BLOCK_FUNCTION(name) memory_block* name (memory_index size, u8 alignment)
typedef MEMORY_ALLOCATE_MEMORY_BLOCK_FUNCTION(memory_allocate_memory_block_function);

#define MEMORY_FREE_MEMORY_BLOCK_FUNCTION(name) void name (memory_block* block)
typedef MEMORY_FREE_MEMORY_BLOCK_FUNCTION(memory_free_memory_block_function);

typedef struct default_memory_arena_allocation_routines
{
	memory_allocate_memory_block_function* allocate_function;
	memory_free_memory_block_function* free_function;
	memory_allocate_memory_block_function* bootstrap_allocate_function;
	memory_free_memory_block_function* bootstrap_free_function;

	memory_index block_size;
	memory_index bootstrap_block_size;
	memory_index minimum_fragment_size;
	u8 block_alignment;
	u8 bootstrap_block_alignment;
} default_memory_arena_allocation_routines;

global_variable default_memory_arena_allocation_routines DefaultMemoryArenaAllocationRoutines = {};

typedef struct memory_arena_default_params
{
	memory_index block_size;
	memory_index minimum_fragment_size;
	u8 block_alignment;
	u8 alignment;
	i8 should_zero;
} memory_arena_default_params;

typedef struct memory_arena
{
	memory_block* current_block;
	memory_index used_memory;
	memory_index block_count;

	memory_allocate_memory_block_function* allocate_function;
	memory_free_memory_block_function* free_function;

	memory_arena_default_params defaults;
} memory_arena;

inline void*
Align(void* ptr, uint8 alignment)
{
	return (void*)((uint8*) ptr + (uint8)(((~((uintptr) ptr)) + 1) & (uint8)(alignment - 1)));
}

inline uint8
AlignOffset(void* ptr, uint8 alignment)
{
	return (uint8)((uint8*) Align(ptr, alignment) - (uint8*) ptr);
}

inline memory_index
RoundToAligned(memory_index size, uint8 alignment)
{
	return size + AlignOffset((void*) ((uint8*)0 + size), alignment);
}

#define CopyArray(source, count, dest) Copy((void*)(source), (count) * sizeof(*(source)), (dest))
inline void*
Copy(void* source, memory_index source_length, void* dest)
{
	uint8* source_ptr = (uint8*) source;
	uint8* dest_ptr	  = (uint8*) dest;

	while (dest_ptr < (uint8*) dest + source_length)
	{
		*dest_ptr++ = *source_ptr++;
	}

	return dest;
}

#define ZeroStruct(object) ZeroSize(&(object), sizeof(object))
#define ZeroArray(source, count) ZeroSize((source), count * sizeof((source)[0]))
inline void
ZeroSize(void* ptr, memory_index size)
{
	u8* byte_ptr = (u8*) ptr;

	while (byte_ptr < (u8*) ptr + size)
	{
		*byte_ptr = 0;
	}
}

inline void
ClearMemoryArena(memory_arena* arena)
{
	if (!arena->free_function)
	{
		arena->free_function = DefaultMemoryArenaAllocationRoutines.free_function;
	}

	memory_block* block		 = arena->current_block;
	memory_block* next_block = NULL;

	while (block)
	{
		next_block = block->previous;
		arena->free_function(block);
		block = next_block;
	}

	arena->block_count = 0;
	arena->used_memory = 0;
}

inline void
ResetMemoryArena(memory_arena* arena)
{
	memory_block* block		 = arena->current_block;

	while(block)
	{
		u8* prev_push_ptr = block->push_ptr;

		void* memory_start = (memory_block*) Align(block->memory, alignof(memory_block)) + 1;

		block->push_ptr = (u8*) Align(memory_start, arena->defaults.alignment);

		memory_index delta = (memory_index)(prev_push_ptr - block->push_ptr);
		block->remaining_space += delta;
		arena->used_memory -= delta;

		block = block->previous;
	}
}

inline void*
PushSize_(memory_arena* arena, memory_index size,
		  u8 passed_alignment = 0, i8 should_zero_memory = -1)
{
	Assert((should_zero_memory <= -1 && should_zero_memory <= 1) || "Non-ternary value passed as should zero memory");

	void* result = NULL;

	if (!arena->allocate_function)
	{
		arena->allocate_function = DefaultMemoryArenaAllocationRoutines.allocate_function;
	}

	u8 alignment	 = (!passed_alignment		 ? arena->defaults.alignment   : passed_alignment);
	i8 should_zero	 = (should_zero_memory == -1 ? arena->defaults.should_zero : should_zero_memory);

	if (!alignment)
	{
		alignment = 1;
	}

	if (should_zero == -1)
	{
		should_zero = 0;
	}

	u8 adjustment = 0;
	memory_index total_size = 0;

	if (arena->current_block)
	{
		adjustment = AlignOffset(arena->current_block->push_ptr, alignment);
		total_size = size + adjustment;
	}

	if (arena->defaults.minimum_fragment_size == 0)
	{
		arena->defaults.minimum_fragment_size = DefaultMemoryArenaAllocationRoutines.minimum_fragment_size;
	}

	if (!arena->current_block || arena->current_block->remaining_space < total_size + arena->defaults.minimum_fragment_size)
	{
		if (arena->current_block && arena->current_block->next)
		{
			arena->current_block = arena->current_block->next;
		}

		else
		{
			if (!arena->defaults.block_size)
			{
				arena->defaults.block_size = DefaultMemoryArenaAllocationRoutines.block_size;
			}

			if (!arena->defaults.block_alignment)
			{
				arena->defaults.block_alignment = DefaultMemoryArenaAllocationRoutines.block_alignment;
			}

			memory_index new_block_size = MAX(arena->defaults.block_size, total_size);
			u8 new_block_alignment		= arena->defaults.block_alignment;

			memory_block* new_block    = arena->allocate_function(new_block_size, new_block_alignment);
			new_block->previous		   = arena->current_block;

			if (arena->current_block)
			{
				arena->current_block->next = new_block;
			}

			arena->current_block	   = new_block;
			++arena->block_count;

			adjustment = AlignOffset(arena->current_block->push_ptr, alignment);
			total_size = size + adjustment;
		}
	}

	memory_block* block = arena->current_block;

	result = Align(block->push_ptr, alignment);

	if (should_zero)
	{
		ZeroSize(result, total_size);
	}

	block->push_ptr += total_size;
	block->remaining_space -= total_size;
	arena->used_memory += total_size;

	return result;
}

inline void*
PushArray_(memory_arena* arena, memory_index size, memory_index count,
		   u8 passed_alignment = 0, i8 should_zero_memory = -1)
{
	Assert((should_zero_memory <= -1 && should_zero_memory <= 1) || "Non-ternary value passed as should zero memory");

	u8 alignment	 = (!passed_alignment		 ? arena->defaults.alignment   : passed_alignment);
	i8 should_zero	 = (should_zero_memory == -1 ? arena->defaults.should_zero : should_zero_memory);

	if (!alignment)
	{
		alignment = 1;
	}

	if (should_zero == -1)
	{
		should_zero = 0;
	}

	memory_index total_size = count * RoundToAligned(size, alignment);
	return PushSize_(arena, total_size, alignment);
}

inline char*
PushString(memory_arena* arena, char* cstring, memory_index length,
		   u8 passed_alignment = 0, i8 should_zero_memory = -1)
{
	Assert((should_zero_memory <= -1 && should_zero_memory <= 1) || "Non-ternary value passed as should zero memory");

	char* result = NULL;

	u8 alignment	 = (!passed_alignment		 ? arena->defaults.alignment   : passed_alignment);
	i8 should_zero	 = (should_zero_memory == -1 ? arena->defaults.should_zero : should_zero_memory);

	if (!alignment)
	{
		alignment = 1;
	}

	if (should_zero == -1)
	{
		should_zero = 0;
	}

	void* memory = PushSize_(arena, sizeof(char) * length + 1);
	result = (char*) Copy((void*) cstring, length, memory);

	if (result)
	{
		result[length] = NULL;
	}

	return result;
}

// NOTE(soimn): these only take should zero parameters
#define PushStruct(arena, type, ...) (type*) PushSize_((arena), sizeof(type), alignof(type), ##__VA_ARGS__)
#define PushArray(arena, type, count, ...) (type*) PushArray_((arena), sizeof(type), count, alignof(type), ##__VA_ARGS__)

// NOTE(soimn): these take all parameters
#define PushSize(arena, size, ...) PushSize_((arena), size, ##__VA_ARGS__)
#define PushAlignedStruct(arena, type, alignment, ...) (type*) PushSize_((arena), sizeof(type), alignment, ##__VA_ARGS__)
#define PushAlignedArray(arena, type, count, alignment, ...) (type*) PushArray_((arena), sizeof(type), count, alignment, ##__VA_ARGS__)

// NOTE(soimn): these only take alignment parameters
#define PushCopy(arena, size, source, ...) Copy((source), size, PushSize(arena, size, ##__VA_ARGS__), size)
#define PushStructCopy(arena, type, source) PushCopy(arena, sizeof(type), source, alignof(type))
#define PushArrayCopy(arena, type, count, source) PushCopy(arena, count * RoundToAligned(sizeof(type), alignof(type)), source, alignof(type))

#define BootstrapPushSize(type, member, ...) (type*) BootstrapPushSize_(sizeof(type), alignof(type), OFFSETOF(type, member), ##__VA_ARGS__)

inline void*
BootstrapPushSize_(memory_index struct_size, u8 struct_alignment,
				   memory_index offset_to_arena, memory_index passed_block_size = 0,
				   u8 passed_block_alignment = 0)
{
	void* result = NULL;

	memory_index block_size = (!passed_block_size	   ? DefaultMemoryArenaAllocationRoutines.bootstrap_block_size		: passed_block_size);
	u8 block_alignment		= (!passed_block_alignment ? DefaultMemoryArenaAllocationRoutines.bootstrap_block_alignment : passed_block_alignment);

	Assert((block_size && block_alignment) || "Defaults not setup properly, bootstrap_block_size and/or bootstrap_block_alignment are NULL");

	memory_arena bootstrap_arena = {};
	bootstrap_arena.allocate_function		 = DefaultMemoryArenaAllocationRoutines.bootstrap_allocate_function;
	bootstrap_arena.free_function			 = DefaultMemoryArenaAllocationRoutines.bootstrap_free_function;
	bootstrap_arena.defaults.block_size		 = block_size;
	bootstrap_arena.defaults.block_alignment = block_alignment;

	result = PushSize(&bootstrap_arena, struct_size, struct_alignment);
	*((memory_arena*)((u8*) result + offset_to_arena)) = bootstrap_arena;

	return result;
}


#define CopyBufferedArray(arena, source, count, dest) CopyBuffered(arena, (void*)(source), (count) * sizeof(*(source)), (dest))
inline void*
CopyBuffered(memory_arena* temp_arena, void* source, memory_index source_length, void* dest)
{
	void* temp_memory = PushSize_(temp_arena, source_length, MEMORY_8BIT_ALIGNED);

	Copy(source, source_length, temp_memory);
	Copy(temp_memory, source_length, dest);

	return dest;
}
