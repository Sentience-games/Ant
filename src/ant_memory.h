#pragma once

#include "ant_shared.h"

#define MEMORY_8BIT_ALIGNED  1
#define MEMORY_16BIT_ALIGNED 2
#define MEMORY_32BIT_ALIGNED 4
#define MEMORY_64BIT_ALIGNED 8

#define MEMORY_DEFAULT_NEW_BLOCK_SIZE KILOBYTES(1)
#define MEMORY_DEFAULT_MAX_FRAGMENT_SIZE BYTES(8)

struct Memory_Block
{
	Memory_Block* next;
	Memory_Block* previous;
    
	void* memory;
	Memory_Index remaining_space;
	U8* push_ptr;
};

struct Memory_Arena
{
	Memory_Block* current_block;
	Memory_Index used_memory;
	Memory_Index block_count;
    
    Memory_Index new_block_size;
    U8 enforced_alignment;
};

inline void*
Align(void* ptr, U8 alignment)
{
	return (void*)((U8*) ptr + (U8)(((~((Uintptr) ptr)) + 1) & (U8)(alignment - 1)));
}

inline U8
AlignOffset(void* ptr, U8 alignment)
{
	return (U8)((U8*) Align(ptr, alignment) - (U8*) ptr);
}

inline Memory_Index
RoundToAligned(Memory_Index size, U8 alignment)
{
	return size + AlignOffset((void*) ((U8*)0 + size), alignment);
}

#define CopyArray(source, count, dest) Copy((void*)(source), (count) * sizeof(*(source)), (dest))
#define CopyStruct(source, dest) Copy((void*)(source), sizeof((source)[0]), (dest))

inline void*
Copy(void* source, Memory_Index source_length, void* dest)
{
	U8* source_ptr = (U8*) source;
	U8* dest_ptr   = (U8*) dest;
    
	while (dest_ptr < (U8*) dest + source_length)
	{
		*dest_ptr++ = *source_ptr++;
	}
    
	return dest;
}

#define ZeroStruct(object) ZeroSize(object, sizeof(object[0]))
#define ZeroArray(source, count) ZeroSize((source), count * sizeof((source)[0]))
inline void
ZeroSize(void* ptr, Memory_Index size)
{
	U8* byte_ptr = (U8*) ptr;
    
	while (byte_ptr < (U8*) ptr + size)
	{
		*(byte_ptr++) = 0;
	}
}

inline void
ClearMemoryArena(Memory_Arena* arena)
{
	Memory_Block* block	  = arena->current_block;
    
	while (block)
	{
		Memory_Block* next_block = block->previous;
        Platform->FreeMemoryBlock(block);
		block = next_block;
	}
    
	arena->block_count = 0;
	arena->used_memory = 0;
}

inline void
ResetMemoryArena(Memory_Arena* arena)
{
	Memory_Block* block = arena->current_block;
    
	while(block)
	{
		U8* prev_push_ptr = block->push_ptr;
        
		void* memory_start = (Memory_Block*) Align(block->memory, alignof(Memory_Block)) + 1;
        
        if (arena->enforced_alignment)
        {
            block->push_ptr = (U8*) Align(memory_start, arena->enforced_alignment);
        }
		
        else
        {
            block->push_ptr = (U8*) Align(memory_start, MEMORY_64BIT_ALIGNED);
        }
        
		Memory_Index delta = (Memory_Index)(prev_push_ptr - block->push_ptr);
		block->remaining_space += delta;
		arena->used_memory -= delta;
        
		block = block->previous;
	}
}

inline void*
PushSize_(Memory_Arena* arena, Memory_Index size,
          U8 alignment = 0, bool should_zero = false)
{
	void* result = NULL;
    
    if (arena->enforced_alignment)
    {
        Assert(alignment == 0);
        alignment = arena->enforced_alignment;
    }
    
    Memory_Index adjustment = 0;
	Memory_Index total_size = 0;
    
	if (arena->current_block)
	{
		adjustment = AlignOffset(arena->current_block->push_ptr, alignment);
		total_size = size + adjustment;
	}
    
	if (!arena->current_block || arena->current_block->remaining_space < total_size + MEMORY_DEFAULT_MAX_FRAGMENT_SIZE)
	{
		if (arena->current_block && arena->current_block->next)
		{
			arena->current_block = arena->current_block->next;
		}
        
		else
		{
            if (arena->new_block_size == 0)
            {
                arena->new_block_size = MEMORY_DEFAULT_NEW_BLOCK_SIZE;
            }
            
			Memory_Index new_block_size = MAX(arena->new_block_size, total_size);
            
			Memory_Block* new_block = Platform->AllocateMemoryBlock(new_block_size);
			new_block->previous	 = arena->current_block;
            
			if (arena->current_block)
			{
				arena->current_block->next = new_block;
			}
            
			arena->current_block = new_block;
			++arena->block_count;
            
			adjustment = AlignOffset(arena->current_block->push_ptr, alignment);
			total_size = size + adjustment;
		}
	}
    
	Memory_Block* block = arena->current_block;
    
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
PushArray_(Memory_Arena* arena, Memory_Index size, Memory_Index count,
		   U8 alignment = 0, bool should_zero_memory = false)
{
	Memory_Index total_size = count * RoundToAligned(size, alignment);
	return PushSize_(arena, total_size, alignment);
}

inline char*
PushCString(Memory_Arena* arena, char* cstring, Memory_Index length,
            U8 passed_alignment = MEMORY_8BIT_ALIGNED, I8 should_zero_memory = -1)
{
	Assert((should_zero_memory <= -1 && should_zero_memory <= 1) || "Non-ternary value passed as should zero memory");
    
	char* result = NULL;
    
	//result = PushString(arena, cstring, length + 1, passed_alignment, should_zero_memory);
    
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
BootstrapPushSize_(Memory_Index struct_size, U8 struct_alignment,
				   Memory_Index offset_to_arena, Memory_Index block_size = 0,
                   U8 enforced_alignment = 0)
{
	void* result = NULL;
    
	Memory_Arena bootstrap_arena = {};
	bootstrap_arena.new_block_size     = block_size;
    bootstrap_arena.enforced_alignment = enforced_alignment;
    
	result = PushSize(&bootstrap_arena, struct_size, struct_alignment);
	*((Memory_Arena*)((U8*) result + offset_to_arena)) = bootstrap_arena;
    
	return result;
}


#define CopyBufferedArray(arena, source, count, dest) CopyBuffered(arena, (void*)(source), (count) * sizeof(*(source)), (dest))
inline void*
CopyBuffered(Memory_Arena* temp_arena, void* source, Memory_Index source_length, void* dest)
{
	void* temp_memory = PushSize_(temp_arena, source_length, MEMORY_8BIT_ALIGNED);
    
	Copy(source, source_length, temp_memory);
	Copy(temp_memory, source_length, dest);
    
	return dest;
}
