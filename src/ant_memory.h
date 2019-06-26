#pragma once

#include "ant_shared.h"

struct Memory_Block
{
	Memory_Block* prev;
	Memory_Block* next;
	U8* push_ptr;
	U64 space;
};

struct Memory_Arena
{
	Memory_Block* current_block;
	UMM block_size;
	U16 block_count;
};

inline void*
Align(void* ptr, U8 alignment)
{
	return (void*)((U8*) ptr + (U8)(((~((UMM) ptr)) + 1) & (U8)(alignment - 1)));
}

inline U8
AlignOffset(void* ptr, U8 alignment)
{
	return (U8)((U8*) Align(ptr, alignment) - (U8*) ptr);
}

inline U8
MaxAlignOfPointer(void* ptr)
{
    U8 result = 1;
    
    U8 masked_ptr = (U8)((UMM) ptr & ~(((UMM) ptr >> 7) << 7));
    
    if (!masked_ptr || masked_ptr == 8) result = 8;
    else if (masked_ptr == 4 || masked_ptr == 12) result = 4;
    else if (masked_ptr % 2 == 0) result = 2;
    
    return result;
}

inline void
ClearArena(Memory_Arena* arena)
{
	Memory_Block* block = arena->current_block;
    
	if (block)
	{
		while (block->next) block = block->next;
        
		do
		{
			Memory_Block* prev_block = block->prev;
			Platform->FreeMemoryBlock(block);
            block = (prev_block ? prev_block : block);
		} while (block->prev);
	}
    
	arena->block_count   = 0;
	arena->current_block = 0;
}

inline void
ResetArena(Memory_Arena* arena)
{
	Memory_Block* block = arena->current_block;
    
	if (block)
	{
		while (block->next) block = block->next;
        
		do
		{
			U8* new_push_ptr = (U8*) Align((void*)(block + 1), 8);
			block->space    += block->push_ptr - new_push_ptr;
            
			block = (block->prev ? block->prev : block);
		} while(block->prev);
	}
}

inline void*
PushSize(Memory_Arena* arena, UMM size, U8 alignment = 1)
{
	void* result = 0;
    
	Assert(arena && size && alignment);
    
	while (!arena->current_block 
		   || arena->current_block->space < size + AlignOffset(arena->current_block->push_ptr, alignment))
	{
		if (arena->current_block && arena->current_block->next)
		{
			arena->current_block = arena->current_block->next;
			continue;
		}
        
		else
		{
			Memory_Block* new_block = Platform->AllocateMemoryBlock(MAX(size + alignment - 1, arena->block_size));
			
			if (arena->current_block) arena->current_block->next = new_block;
			new_block->prev = arena->current_block;
            
			arena->current_block = new_block;
			++arena->block_count;
		}
	}
    
	UMM total_size = size + AlignOffset(arena->current_block->push_ptr, alignment);
	
	result = Align(arena->current_block->push_ptr, alignment);
    
	arena->current_block->push_ptr += total_size;
	arena->current_block->space    -= total_size;
    
	return result;
}

#define PushStruct(arena, type) (type*) PushSize(arena, sizeof(type), alignof(type))
#define PushArray(arena, type, count) (type*) PushSize(arena, (sizeof(type) + AlignOffset(((type*) 0) + 1, alignof(type))) * (count), alignof(type))

inline void
Copy(void* source, void* dest, UMM size)
{
	Assert(source && dest);
    
	U8* bsource = (U8*) source;
	U8* bdest   = (U8*) dest;
    
	if (size)
	{
		while (bsource < (U8*) source + size) *(bdest++) = *(bsource++);
	}
}

#define CopyStruct(source, dest) Copy((void*) (source), (void*) (dest), sizeof(*(source)))
#define CopyArray(source, dest, count) Copy((void*) (source), (void*) (dest), sizeof(*(source)) * (count))

inline void
ZeroSize(void* ptr, UMM size)
{
	U8* bptr = (U8*) ptr;
    
	while (bptr < (U8*) ptr + size) *(bptr++) = 0;
}

#define ZeroStruct(type) ZeroSize(type, sizeof(type[0]))
#define ZeroArray(type, count) ZeroSize(type, sizeof(type[0]) * (count))

inline void
MemSet(void* ptr, UMM size, U8 value)
{
    U8* bptr = (U8*) ptr;
    
    while (bptr < (U8*) ptr + size) *(bptr++) = value;
}

inline bool
IsZero(void* ptr, UMM size)
{
    U8* bptr = (U8*) ptr;
    
    while (bptr < (U8*) ptr + size && !(*bptr)) ++bptr;
    
    return (bptr == (U8*) ptr + size);
}

#define IsStructZero(structure) IsZero(structure, sizeof((structure)[0]))
#define IsArrayZero(array, count) IsZero(array, sizeof((array)[0]) * (count))

inline bool
CompareSize(void* p0, void* p1, UMM size)
{
    U8* bp0 = (U8*) p0;
    U8* bp1 = (U8*) p1;
    
    while (bp0 < (U8*) p0 + size && *bp0 == *bp1)
    {
        ++bp0;
        ++bp1;
    }
    
    return (bp0 == (U8*) p0 + size);
}

#define CompareStruct(s0, s1) CompareSize(s0, s1, sizeof((s0)[0]))
#define CompareArray(s0, s1, count) CompareSize(s0, s1, sizeof((s0)[0]) * (count))

inline void*
BootstrapPushSize_(UMM container_size, U8 container_alignment, UMM offset_to_arena_member, UMM block_size)
{
	void* result = 0;
    
    U8 adjustment         = AlignOffset((Memory_Arena*)0 + 1, container_alignment);
	UMM total_header_size = sizeof(Memory_Arena) + adjustment + container_size;
    
    Memory_Arena bootstrap_arena = {};
	bootstrap_arena.block_size   = block_size;
    
    bootstrap_arena.current_block = Platform->AllocateMemoryBlock(total_header_size + block_size);
    ++bootstrap_arena.block_count;
    
    Memory_Arena* stored_arena = PushStruct(&bootstrap_arena, Memory_Arena);
    *stored_arena = bootstrap_arena;
    
	result = PushSize(stored_arena, container_size, container_alignment);
	*((Memory_Arena**) ((U8*) result + offset_to_arena_member)) = stored_arena;
    
	return result;
}

#define BootstrapPushSize(container, member, block_size) (container*) BootstrapPushSize_(sizeof(container), alignof(container), OFFSETOF(container, member), block_size)