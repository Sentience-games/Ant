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

inline UMM
RoundSize(UMM size, U8 alignment)
{
    return size + AlignOffset((U8*)size, alignment);
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
    
    UMM total_size = (arena->current_block ? size + AlignOffset(arena->current_block->push_ptr, alignment) : 0);
    
    if (!arena->current_block || arena->current_block->space < total_size)
    {
        if (arena->current_block && arena->current_block->next)
        {
            if (arena->current_block->next->space >= size)
            {
                arena->current_block = arena->current_block->next;
            }
            
            else
            {
                Memory_Block* new_block = Platform->AllocateMemoryBlock(size + alignment - 1);
                
                Memory_Block* next = arena->current_block->next;
                
                arena->current_block->next = new_block;
                new_block->prev            = arena->current_block;
                
                new_block->next = next;
                next->prev = new_block;
                
                arena->current_block = new_block;
                ++arena->block_count;
            }
        }
        
        else
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
        
        total_size = size + AlignOffset(arena->current_block->push_ptr, alignment);
    }
    
    
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
    Bucket_Array_Block* prev;
    U32 offset;
    U32 space;
};

struct Bucket_Array
{
    Memory_Arena* arena;
    
    Bucket_Array_Block* first_block;
    Bucket_Array_Block* current_block;
    
    U32 element_size;
    U32 block_size;
    U32 block_count;
};

inline Bucket_Array
BucketArray(Memory_Arena* arena, UMM element_size, U32 block_size)
{
    Assert(element_size <= U32_MAX);
    
    Bucket_Array result = {};
    result.arena        = arena;
    result.element_size = (U32)element_size;
    result.block_size   = block_size;
    
    return result;
}

#define BUCKET_ARRAY(arena, type, block_size) BucketArray(arena, RoundSize(sizeof(type), alignof(type)), block_size)

inline void*
ElementAt(Bucket_Array* array, UMM index)
{
    void* result = 0;
    
    UMM array_size = (array->block_count - 1) * array->block_size + array->current_block->offset;
    if (index < array_size)
    {
        Bucket_Array_Block* scan = array->first_block;
        
        UMM block_index  = index / array->block_size;
        U32 block_offset = index % array->block_size;
        
        if (block_index == array->block_count - 1)
        {
            scan = array->current_block;
        }
        
        else if (block_index <= array->block_count / 2)
        {
            for (U32 i = 0; i < block_index, scan; ++i)
            {
                scan = scan->next;
            }
        }
        
        else
        {
            scan = array->current_block;
            
            for (U32 i = 0; i < array->block_count - (block_index + 1), scan; ++i)
            {
                scan = scan->prev;
            }
        }
        
        if (scan)
        {
            result = (U8*)(scan + 1) + array->element_size * block_offset;
        }
    }
    
    return result;
}

inline void*
PushElement(Bucket_Array* array)
{
    void* result = 0;
    
    if (!array->first_block || !array->current_block->space)
    {
        Assert(array->block_count < U32_MAX);
        
        UMM block_size = sizeof(Bucket_Array_Block) + array->element_size * array->block_size;
        Bucket_Array_Block* new_block = (Bucket_Array_Block*)PushSize(array->arena, block_size, alignof(Bucket_Array_Block));
        *new_block = {};
        
        if (array->first_block)
        {
            array->current_block->next = new_block;
            new_block->prev = array->current_block;
        }
        
        else
        {
            array->first_block = new_block;
        }
        
        array->current_block = new_block;
        ++array->block_count;
    }
    
    result = (U8*)(array->current_block + 1) + array->element_size * array->current_block->offset;
    ++array->current_block->offset;
    --array->current_block->space;
    
    return result;
}

struct Bucket_Array_Iterator
{
    Bucket_Array_Block* current_block;
    UMM current_index;
    U32 element_size;
    U32 block_size;
    B32 iterate_backwards;
    void* current;
};

inline Bucket_Array_Iterator
Iterate(Bucket_Array* array, Enum8(FORWARD_OR_BACKWARD) direction = FORWARD)
{
    Bucket_Array_Iterator iterator = {};
    
    if (array->current_block)
    {
        iterator.element_size      = array->element_size;
        iterator.block_size        = array->block_size;
        iterator.iterate_backwards = (direction != FORWARD);
        
        if (direction == FORWARD)
        {
            iterator.current_block = array->first_block;
            iterator.current_index = 0;
            iterator.current = array->first_block + 1;
        }
        
        else
        {
            Assert(direction == BACKWARD);
            
            iterator.current_block = array->current_block;
            
            U32 offset = array->current_block->offset - 1;
            iterator.current_index = (array->block_count - 1) * array->block_size + offset;
            iterator.current = (U8*)(array->current_block + 1) + array->element_size * offset;
        }
    }
    
    return iterator;
}

inline void
Advance(Bucket_Array_Iterator* iterator)
{
    iterator->current = 0;
    
    ++iterator->current_index;
    
    U32 offset = iterator->current_index % iterator->block_size;
    
    if (offset == 0)
    {
        iterator->current_block = iterator->current_block->next;
    }
    
    if (iterator->current_block)
    {
        iterator->current = (U8*)(iterator->current_block + 1) + iterator->element_size * offset;
    }
}

struct Free_List_Bucket_Array_Block
{
    Free_List_Bucket_Array_Block* next;
    U32 offset;
    U32 space;
};

struct Free_List_Bucket_Array
{
    void** free_list;
    
    Memory_Arena* arena;
    Free_List_Bucket_Array_Block* first_block;
    Free_List_Bucket_Array_Block* current_block;
    
    U32 element_size;
    U32 block_size;
    U32 block_count;
};

inline Free_List_Bucket_Array
FreeListBucketArray(Memory_Arena* arena, UMM element_size, U32 block_size)
{
    Assert(element_size <= U32_MAX);
    Assert(element_size >= sizeof(void*));
    
    Free_List_Bucket_Array result = {};
    result.arena = arena;
    result.element_size = (U32)element_size;
    result.block_size = block_size;
    
    return result;
}

#define FREE_LIST_BUCKET_ARRAY(arena, type, block_size) FreeListBucketArray(arena, RoundSize(sizeof(type), alignof(type)), block_size)

inline void*
PushElement(Free_List_Bucket_Array* array)
{
    void* result = 0;
    
    if (array->free_list)
    {
        result = array->free_list;
        array->free_list = (void**)*array->free_list;
    }
    
    else
    {
        if (!array->first_block || !array->current_block->space)
        {
            Assert(array->block_count < U32_MAX);
            
            UMM block_size = sizeof(Free_List_Bucket_Array_Block) + array->element_size * array->block_size;
            Free_List_Bucket_Array_Block* new_block = (Free_List_Bucket_Array_Block*)PushSize(array->arena, block_size, alignof(Free_List_Bucket_Array_Block));
            *new_block = {};
            
            if (array->first_block)
            {
                array->current_block->next = new_block;
            }
            
            else
            {
                array->first_block = new_block;
            }
            
            array->current_block = new_block;
            ++array->block_count;
        }
        
        result = (U8*)(array->current_block + 1) + array->element_size * array->current_block->offset;
        ++array->current_block->offset;
        --array->current_block->space;
    }
    
    return result;
}

inline void
RemoveElement(Free_List_Bucket_Array* array, void* element)
{
    bool is_valid = false;
    
    if (element)
    {
        U8* element_u8 = (U8*)element;
        Free_List_Bucket_Array_Block* scan = array->first_block;
        
        while (scan)
        {
            U8* block = (U8*)(scan + 1);
            UMM offset = element_u8 - block;
            if (block <= element_u8 && element_u8 < block + array->block_size)
            {
                if (offset % array->element_size == 0 && (scan != array->current_block || offset < array->current_block->offset))
                {
                    is_valid = true;
                }
                
                break;
            }
            
            scan = scan->next;
        }
    }
    
    Assert(is_valid);
    
    if (is_valid)
    {
        *(void**)element = array->free_list;
        array->free_list = (void**)element;
    }
}

struct Free_List_Variable_Bucket_Array_Free_List_Entry
{
    Free_List_Variable_Bucket_Array_Free_List_Entry* next;
    U32 size;
    U16 offset;
    U16 block_index;
};

struct Free_List_Variable_Bucket_Array_Block
{
    Free_List_Variable_Bucket_Array_Block* next;
    U32 offset;
    U32 space;
};

struct Free_List_Variable_Bucket_Array
{
    Memory_Arena* arena;
    
    Free_List_Variable_Bucket_Array_Free_List_Entry* free_list;
    Free_List_Variable_Bucket_Array_Block* first_block;
    Free_List_Variable_Bucket_Array_Block* current_block;
    U16 base_element_size;
    U16 base_block_size;
    U16 block_count;
    U16 largest_free;
};

inline Free_List_Variable_Bucket_Array
FreeListVariableBucketArray(Memory_Arena* arena, UMM base_element_size, U32 base_block_size)
{
    Assert(base_element_size <= U16_MAX);
    Assert(base_block_size   <= U16_MAX);
    Assert(base_element_size >= sizeof(Free_List_Variable_Bucket_Array_Free_List_Entry));
    
    Free_List_Variable_Bucket_Array result = {};
    result.arena             = arena;
    result.base_element_size = (U16)base_element_size;
    result.base_block_size   = (U16)base_block_size;
    
    return result;
}

#define FREE_LIST_VARIABLE_BUCKET_ARRAY(arena, type, base_block_size) FreeListVariableBucketArray(arena, RoundSize(sizeof(type), alignof(type)), base_block_size)

inline void*
PushElement(Free_List_Variable_Bucket_Array* array, U16 size = 1)
{
    void* result = 0;
    
    if (array->free_list)
    {
        U16 best_size = U16_MAX;
        Free_List_Variable_Bucket_Array_Free_List_Entry* prev_to_best_entry = 0;
        Free_List_Variable_Bucket_Array_Free_List_Entry* best_entry         = 0;
        
        Free_List_Variable_Bucket_Array_Free_List_Entry* prev_to_scan = 0;
        Free_List_Variable_Bucket_Array_Free_List_Entry* scan         = array->free_list;
        
        while (scan)
        {
            if (scan->size >= size && scan->size < best_size)
            {
                prev_to_best_entry = prev_to_scan;
                best_entry = scan;
                
                best_size = (U16)scan->size;
                
                if (scan->size == size)
                {
                    break;
                }
            }
            
            prev_to_scan = scan;
            scan = scan->next;
        }
        
        if (best_entry)
        {
            result = best_entry;
            
            if (prev_to_best_entry)
            {
                prev_to_best_entry->next = best_entry->next;
            }
            
            if (best_size != size)
            {
                U16 new_size = best_size - size;
                
                Free_List_Variable_Bucket_Array_Free_List_Entry* new_entry = (Free_List_Variable_Bucket_Array_Free_List_Entry*)((U8*)(best_entry) + array->base_element_size * size);
                
                new_entry->next   = array->free_list;
                new_entry->offset = best_entry->offset + size;
                new_entry->size   = new_size;
                new_entry->block_index = best_entry->block_index;
                
                array->largest_free = Max(array->largest_free, new_size);
                
                array->free_list = new_entry;
            }
        }
        
    }
    
    if (!result)
    {
        if (!array->current_block || array->current_block->space < size)
        {
            UMM block_size = Max(array->base_block_size, size);
            
            if (array->current_block)
            {
                Free_List_Variable_Bucket_Array_Free_List_Entry* new_entry = (Free_List_Variable_Bucket_Array_Free_List_Entry*)((U8*)(array->current_block + 1) + array->base_element_size * array->current_block->offset);
                
                new_entry->next   = 0;
                new_entry->offset = (U16)array->current_block->offset;
                new_entry->size   = (U16)array->current_block->space;
                new_entry->block_index = array->block_count - 1;
                
                array->free_list = new_entry;
            }
            
            Free_List_Variable_Bucket_Array_Block* new_block = (Free_List_Variable_Bucket_Array_Block*)PushSize(array->arena, sizeof(Free_List_Variable_Bucket_Array_Block) + block_size, alignof(Free_List_Variable_Bucket_Array_Block));
            
            new_block->next   = 0;
            new_block->offset = 0;
            new_block->space  = (U16)block_size;
            
            if (array->first_block)
            {
                array->current_block->next = new_block;
            }
            
            else
            {
                array->first_block = new_block;
            }
            
            array->current_block = new_block;
            ++array->block_count;
        }
        
        result = (U8*)(array->current_block + 1) + array->base_element_size * array->current_block->offset;
        array->current_block->offset += size;
        array->current_block->space  -= size;
    }
    
    return result;
}

inline void
RemoveElement(Free_List_Variable_Bucket_Array* array, void* element, U32 size)
{
    bool is_valid = false;
    
    UMM offset      = 0;
    U16 block_index = 0; 
    
    if (element)
    {
        U8* element_u8 = (U8*)element;
        Free_List_Variable_Bucket_Array_Block* scan = array->first_block;
        
        for (; scan; ++block_index)
        {
            U8* block = (U8*)(scan + 1);
            if (block <= element_u8 && element_u8 + size <= block + scan->offset)
            {
                if ((element_u8 - block) % array->base_element_size == 0)
                {
                    is_valid = true;
                }
                
                break;
            }
            
            scan = scan->next;
        }
    }
    
    Assert(is_valid);
    
    if (is_valid)
    {
        Free_List_Variable_Bucket_Array_Free_List_Entry* new_entry = (Free_List_Variable_Bucket_Array_Free_List_Entry*)element;
        
        new_entry->next        = array->free_list;
        new_entry->offset      = (U16)offset;
        new_entry->size        = size;
        new_entry->block_index = block_index;
        
        Free_List_Variable_Bucket_Array_Free_List_Entry* prev_to_scan = 0;
        Free_List_Variable_Bucket_Array_Free_List_Entry* scan         = array->free_list;
        
        bool found_front = false;
        bool found_back  = false;
        
        while (scan && !(found_front && found_back))
        {
            if (scan->block_index == new_entry->block_index)
            {
                if (scan->offset + scan->size == new_entry->offset)
                {
                    if (prev_to_scan)
                    {
                        prev_to_scan->next = scan->next;
                    }
                    
                    new_entry->offset = scan->offset;
                    new_entry->size  += scan->size;
                    
                    found_front = true;
                }
                
                else if (new_entry->offset + new_entry->size == scan->offset)
                {
                    if (prev_to_scan)
                    {
                        prev_to_scan->next = scan->next;
                    }
                    
                    new_entry->size += scan->size;
                    
                    found_back = true;
                }
            }
            
            prev_to_scan = scan;
            scan = scan->next;
        }
        
        array->free_list = new_entry;
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