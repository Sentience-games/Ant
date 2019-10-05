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

inline U8*
Align(void* ptr, U8 alignment)
{
	return ((U8*) ptr + (U8)(((~((UMM) ptr)) + 1) & (U8)(alignment - 1)));
}

inline U8
AlignOffset(void* ptr, U8 alignment)
{
	return (U8)((U8*) Align(ptr, alignment) - (U8*) ptr);
}

inline void
Copy(void* source, void* dest, UMM size)
{
    Assert(size);
    
    U8* bsource = (U8*) source;
    U8* bdest   = (U8*) dest;
    
    while (bsource < (U8*) source + size)
    {
        *(bdest++) = *(bsource++);
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

#define ZeroStruct(type) (*(type) = {})
#define ZeroArray(type, count) ZeroSize(type, sizeof((type)[0]) * (count))

inline void
ClearArena(Memory_Arena* arena)
{
    Memory_Block* block = arena->current_block;
    
    while (block && block->next)
    {
        block = block->next;
    }
    
    while (block)
    {
        Memory_Block* temp_ptr = block->prev;
        
        Platform->FreeMemoryBlock(block);
        
        block = temp_ptr;
    }
    
    arena->current_block = 0;
    arena->block_count   = 0;
}

inline void
ResetArena(Memory_Arena* arena)
{
    Memory_Block* block = arena->current_block;
    
    while (block && block->next)
    {
        block = block->next;
    }
    
    while (block)
    {
        U8* new_push_ptr = Align(block + 1, 8);
        block->space    += block->push_ptr - new_push_ptr;
        block->push_ptr  = new_push_ptr;
        
        block = block->prev;
    }
}

inline void*
PushSize(Memory_Arena* arena, UMM size, U8 alignment = 1)
{
    void* result = 0;
    
    Assert(size);
    Assert(alignment && (alignment == 1 || alignment == 2 || alignment == 4 || alignment == 8));
    
    if (!arena->current_block || arena->current_block->space < size + AlignOffset(arena->current_block->push_ptr, alignment))
    {
        bool should_not_allocate_next = false;
        
        if (arena->current_block && arena->current_block->next)
        {
            if (arena->current_block->next->space >= size)
            {
                arena->current_block = arena->current_block->next;
                
                should_not_allocate_next = true;
            }
            
            else
            {
                Memory_Block* blocks_to_dealloc = arena->current_block->next;
                
                while (blocks_to_dealloc)
                {
                    Memory_Block* temp_ptr = blocks_to_dealloc->next;
                    
                    Platform->FreeMemoryBlock(blocks_to_dealloc);
                    
                    --arena->block_count;
                    blocks_to_dealloc = temp_ptr;
                }
            }
        }
        
        if (!should_not_allocate_next)
        {
            UMM block_size = Max(size + alignment - 1, arena->block_size);
            Memory_Block* new_block = Platform->AllocateMemoryBlock(block_size);
            
            if (arena->current_block)
            {
                arena->current_block->next = new_block;
                new_block->prev            = arena->current_block;
            }
            
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
#define PushArray(arena, type, count) (type*) PushSize(arena, (count) * (sizeof(type) + AlignOffset(((type*)0) + 1, alignof(type))), alignof(type))

/// Memory management utility structures and functions

struct Bucket_Array_Block
{
    Bucket_Array_Block* next;
    U32 current_offset;
    U32 current_space;
};

struct Bucket_Array
{
    Memory_Arena* arena;
    Bucket_Array_Block* first_block;
    Bucket_Array_Block* current_block;
    U16 block_count;
    U16 element_size;
    U32 block_size;
};

#define BUCKET_ARRAY(arena, type, block_size) {arena, 0, 0, 0, sizeof(type), block_size}

inline void*
PushElement(Bucket_Array* array)
{
    void* result = 0;
    
    if (!array->current_block || !array->current_block->current_space)
    {
        UMM memory_block_size = sizeof(Bucket_Array_Block) + array->element_size * array->block_size;
        
        Bucket_Array_Block* new_block = (Bucket_Array_Block*)PushSize(array->arena, memory_block_size, alignof(Bucket_Array_Block));
        
        if (array->current_block)
        {
            array->current_block->next = new_block;
        }
        
        array->current_block = array->current_block->next;
        *array->current_block = {};
    }
    
    result = (U8*)(array->current_block + 1) + array->element_size * array->current_block->current_offset;
    ++array->current_block->current_offset;
    --array->current_block->current_space;
    
    return result;
}

struct Bucket_Array_Iterator
{
    Bucket_Array_Block* current_block;
    UMM current_index;
    U32 current_offset;
    U16 element_size;
    void* current;
};

inline Bucket_Array_Iterator
Iterate(Bucket_Array* array)
{
    Bucket_Array_Iterator iterator = {};
    
    iterator.current_block = array->first_block;
    iterator.element_size  = array->element_size;
    
    iterator.current = (array->first_block ? array->first_block + 1 : 0);
    
    return iterator;
}

inline void
Advance(Bucket_Array_Iterator* iterator)
{
    Assert(iterator->current);
    
    ++iterator->current_index;
    ++iterator->current_offset;
    iterator->current = (U8*)iterator->current + iterator->element_size;
    
    if (iterator->current_offset >= iterator->current_block->current_offset)
    {
        iterator->current_offset = 0;
        iterator->current_block  = iterator->current_block->next;
        
        iterator->current = (iterator->current_block ? iterator->current_block + 1 : 0);
    }
}

struct Temporary_Memory
{
    Memory_Arena* arena;
    Memory_Block* first_block;
};

inline Temporary_Memory
BeginTempMemory(Memory_Arena* arena)
{
    Temporary_Memory result = {};
    
    result.arena       = arena;
    result.first_block = arena->current_block;
    
    return result;
}

inline void
EndTempMemory(Temporary_Memory* temp_memory)
{
    if (temp_memory->first_block)
    {
        Memory_Block* last_block = temp_memory->arena->current_block;
        
        while (last_block && last_block != temp_memory->first_block)
        {
            Memory_Block* temp_ptr = last_block->prev;
            
            Platform->FreeMemoryBlock(last_block);
            
            --temp_memory->arena->block_count;
            last_block = temp_ptr;
        }
        
        if (last_block)
        {
            last_block->next = 0;
        }
    }
    
    else
    {
        ClearArena(temp_memory->arena);
    }
    
    *temp_memory = {};
}