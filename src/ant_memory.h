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
    Bucket_Array_Block* prev;
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

inline Bucket_Array
BucketArray(Memory_Arena* arena, UMM element_size, U32 block_size)
{
    Assert(element_size < U16_MAX);
    
    Bucket_Array array = {};
    array.arena        = arena;
    array.element_size = (U16)element_size;
    array.block_size   = block_size;
    
    return array;
}
#define BUCKET_ARRAY(arena, type, block_size) BucketArray(arena, RoundSize(sizeof(type), alignof(type)), block_size)

inline void*
ElementAt(Bucket_Array* array, UMM index)
{
    UMM block_index  = index / array->block_size;
    U32 block_offset = index % array->block_size;
    
    void* result = 0;
    
    if (block_index < array->block_count)
    {
        Bucket_Array_Block* block = 0;
        
        if (block_index <= array->block_count - block_index)
        {
            block = array->first_block;
            
            for (U32 i = 0; i < block_index; ++i)
            {
                block = block->next;
            }
        }
        
        else
        {
            block = array->current_block;
            
            for (U32 i = 0; i < array->block_count - block_index; ++i)
            {
                block = block->prev;
            }
        }
        
        result = (U8*)(block + 1) + array->element_size * block_offset;
    }
    
    return result;
}

inline void*
PushElement(Bucket_Array* array)
{
    void* result = 0;
    
    if (!array->current_block || !array->current_block->current_space)
    {
        UMM memory_block_size = sizeof(Bucket_Array_Block) + array->element_size * array->block_size;
        
        Bucket_Array_Block* new_block = (Bucket_Array_Block*)PushSize(array->arena, memory_block_size, alignof(Bucket_Array_Block));
        *new_block = {};
        
        if (array->current_block)
        {
            array->current_block->next = new_block;
        }
        
        new_block->prev      = array->current_block;
        array->current_block = new_block;
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
    B16 iterate_backwards;
    void* current;
};

inline Bucket_Array_Iterator
Iterate(Bucket_Array* array, bool iterate_backwards = false)
{
    Bucket_Array_Iterator iterator = {};
    
    if (array->current_block)
    {
        iterator.current_block = array->first_block;
        iterator.element_size  = array->element_size;
        iterator.iterate_backwards = iterate_backwards;
        
        if (!iterate_backwards)
        {
            iterator.current = array->first_block + 1;
        }
        
        else
        {
            iterator.current_index = (array->block_count - 1) * array->block_size + array->current_block->current_offset;
            iterator.current = (U8*)(array->current_block + 1) + array->element_size * array->current_block->current_offset;
        }
    }
    
    return iterator;
}

inline void
Advance(Bucket_Array_Iterator* iterator)
{
    Assert(iterator->current);
    
    if (!iterator->iterate_backwards)
    {
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
    
    else
    {
        if (iterator->current_index)
        {
            UMM prev_index = iterator->current_index--;
            U32 block_size = iterator->current_block->current_offset + iterator->current_block->current_space;
            
            if (prev_index / block_size != iterator->current_index / block_size)
            {
                iterator->current_block = iterator->current_block->prev;
            }
            
            iterator->current = (U8*)(iterator->current_block + 1) + iterator->element_size * (iterator->current_index % block_size);
        }
        
        else
        {
            iterator->current = 0;
        }
    }
}

struct Free_List_Entry
{
    Free_List_Entry* next;
    UMM size;
};

struct Free_List_Variable_Bucket_Array
{
    Free_List_Entry* free_list;
    Bucket_Array bucket_array;
};

inline Free_List_Variable_Bucket_Array
FreeListVariableBucketArray(Memory_Arena* arena, UMM base_element_size, U32 block_size)
{
    Assert(base_element_size < U16_MAX);
    Assert(base_element_size >= sizeof(Free_List_Entry));
    
    Free_List_Variable_Bucket_Array array = {};
    array.bucket_array.arena        = arena;
    array.bucket_array.element_size = (U16)base_element_size;
    array.bucket_array.block_size   = block_size;
    
    return array;
}
#define FREE_LIST_VARIABLE_BUCKET_ARRAY(arena, type, block_size) FreeListVariableBucketArray(arena, RoundSize(sizeof(type), alignof(type)), block_size)
inline void*
PushElement(Free_List_Variable_Bucket_Array* array, U32 count = 1)
{
    bool allocate_new = true;
    
    void* result = 0;
    
    Bucket_Array* bucket_array = &array->bucket_array;
    
    if (array->free_list)
    {
        Free_List_Entry* prev_scan = 0;
        Free_List_Entry* scan      = array->free_list;
        
        Free_List_Entry* prev_best_fit = 0;
        Free_List_Entry* best_fit      = 0;
        
        UMM best_fit_size = 0;
        
        while (scan)
        {
            if (scan->size >= count && (!best_fit || scan->size < best_fit_size))
            {
                prev_best_fit = prev_scan;
                best_fit      = scan;
                best_fit_size = scan->size;
            }
            
            prev_scan = scan;
            scan      = scan->next;
        }
        
        if (best_fit)
        {
            prev_best_fit->next = best_fit->next;
            
            result = best_fit;
        }
    }
    
    if (allocate_new && count <= bucket_array->block_size)
    {if (!bucket_array->current_block || bucket_array->current_block->current_space < count)
        {
            if (bucket_array->current_block)
            {
                U16 element_size = bucket_array->element_size;
                U32 offset = bucket_array->current_block->current_offset;
                U32 space  = bucket_array->current_block->current_space;
                
                bucket_array->current_block->current_offset += space;
                bucket_array->current_block->current_space = 0;
                
                Free_List_Entry* new_entry = (Free_List_Entry*)((U8*)(bucket_array->current_block + 1) + element_size * offset);
                
                new_entry->next = array->free_list;
                new_entry->size = space;
                
                array->free_list = new_entry;
            }
            
            UMM block_size = bucket_array->element_size * bucket_array->block_size;
            
            Bucket_Array_Block* new_block = (Bucket_Array_Block*)PushSize(bucket_array->arena, block_size, alignof(Bucket_Array_Block));
            *new_block = {};
            
            new_block->current_offset = count;
            new_block->current_space  = bucket_array->block_size - count;
            
            if (bucket_array->current_block)
            {
                bucket_array->current_block->next = new_block;
                new_block->prev = bucket_array->current_block;
            }
            
            else
            {
                bucket_array->first_block = new_block;
            }
            
            bucket_array->current_block = new_block;
        }
        
        result = bucket_array->current_block + 1;
    }
    
    return result;
}

inline void
RemoveElement(Free_List_Variable_Bucket_Array* array, void* element, U32 size = 1)
{
    bool is_valid = false;
    
    U8* element_u8 = (U8*)element;
    Bucket_Array* bucket_array = &array->bucket_array;
    
    Bucket_Array_Block* scan = bucket_array->first_block;
    U16 element_size = bucket_array->element_size;
    while (scan)
    {
        if (element_u8 >= (U8*)(scan + 1) && element_u8 < (U8*)(scan + 1) + element_size * scan->current_offset)
        {
            U32 offset = (U32)(element_u8 - (U8*)(scan + 1));
            
            if (offset % element_size == 0 && offset + size <= scan->current_offset)
            {
                is_valid = true;
            }
            
            break;
        }
        
        scan = scan->next;
    }
    
    Assert(is_valid);
    
    if (is_valid)
    {
        Free_List_Entry* new_entry = (Free_List_Entry*)element;
        
        new_entry->next = array->free_list;
        new_entry->size = size;
        
        array->free_list = new_entry;
    }
}

inline Bucket_Array_Iterator
Iterate(Free_List_Variable_Bucket_Array* array, bool iterate_backwards)
{
    return Iterate(&array->bucket_array, iterate_backwards);
}

struct Fixed_Bucket_Array
{
    Memory_Arena* arena;
    U64** block_table;
    U32 max_block_count;
    U32 block_count;
    U32 element_size;
    U32 block_size;
};

inline Fixed_Bucket_Array
FixedBucketArray(Memory_Arena* arena, UMM element_size, U32 block_size, U32 max_block_count)
{
    Assert(element_size < U16_MAX);
    
    Fixed_Bucket_Array array = {};
    array.arena           = arena;
    array.max_block_count = max_block_count;
    array.element_size    = (U16)element_size;
    array.block_size      = block_size;
    
    array.block_table = PushArray(arena, U64*, max_block_count);
    
    return array;
}

#define FIXED_BUCKET_ARRAY(arena, type, block_size, max_block_count) FixedBucketArray(arena, RoundSize(sizeof(type), alignof(type)), block_size, max_block_count)

inline void*
ElementAt(Fixed_Bucket_Array* array, UMM index)
{
    UMM block_index  = index / array->block_size;
    U32 block_offset = index % array->block_size;
    
    void* result = 0;
    
    if (block_offset < array->block_count)
    {
        result = (U8*)(array->block_table[block_index] + 1) + array->element_size * block_offset;
    }
    
    return result;
}

inline void*
PushElement(Fixed_Bucket_Array* array)
{
    void* result = 0;
    
    if (!array->block_count)
    {
        if (array->max_block_count)
        {
            array->block_table[0] = (U64*)PushSize(array->arena, sizeof(U64) + array->element_size * array->block_size, alignof(U64));
            ++array->block_count;
            
            result = array->block_table[0] + 1;
            
            *((U32*)array->block_table[0] + 0) = array->block_size - 1;
            *((U32*)array->block_table[0] + 1) = 1;
        }
    }
    
    else
    {
        U32* current_offset = (U32*)array->block_table[array->block_count - 1] + 0;
        U32* current_space  = (U32*)array->block_table[array->block_count - 1] + 1;
        
        if (*current_space)
        {
            result = (U8*)(array->block_table[array->block_count - 1] + 1) + array->element_size * *current_offset;
            
            --*current_space;
            ++*current_offset;
        }
        
        else if (array->block_count < array->max_block_count)
        {
            ++array->block_count;
            
            array->block_table[array->block_count - 1] = (U64*)PushSize(array->arena, sizeof(U64) + array->element_size * array->block_size, alignof(U64));
            
            result = array->block_table[array->block_count - 1] + 1;
            
            *((U32*)array->block_table[array->block_count - 1] + 0) = array->block_size - 1;
            *((U32*)array->block_table[array->block_count - 1] + 1) = 1;
        }
    }
    
    return result;
}

struct Fixed_Bucket_Array_Iterator
{
    U64** block_table;
    UMM current_index;
    U32 block_count;
    U32 block_size;
    U32 element_size;
    B32 iterate_backwards;
    void* current;
};

inline Fixed_Bucket_Array_Iterator
Iterate(Fixed_Bucket_Array* array, bool iterate_backwards = false)
{
    Fixed_Bucket_Array_Iterator iterator = {};
    
    if (array->block_count)
    {
        iterator.block_table  = array->block_table;
        iterator.block_count  = array->block_count;
        iterator.block_size   = array->block_size;
        iterator.element_size = array->element_size;
        iterator.iterate_backwards = iterate_backwards;
        
        if (!iterate_backwards)
        {
            iterator.current = array->block_table[0] + 1;
        }
        
        else
        {
            U32* current_offset = (U32*)(array->block_table[array->block_count - 1]) + 1;
            iterator.current_index = (array->block_count - 1) * array->block_size + *current_offset;
            
            U32 block_offset = iterator.current_index % array->block_size;
            
            iterator.current = (U64*)(array->block_table[array->block_count - 1] + 1) + array->element_size * block_offset;
        }
    }
    
    return iterator;
}

inline void
Advance(Fixed_Bucket_Array_Iterator* iterator)
{
    ++iterator->current_index;
    
    iterator->current = 0;
    
    U32 block_offset = iterator->current_index % iterator->block_size;
    
    if (!iterator->iterate_backwards)
    {
        UMM block_index  = iterator->current_index / iterator->block_size;
        
        if (block_index < iterator->block_count)
        {
            iterator->current = (U8*)(iterator->block_table[block_index] + 1) + iterator->element_size * block_offset;
        }
    }
    
    else if (iterator->current_index)
    {
        --iterator->current_index;
        
        iterator->current = (U8*)(iterator->block_table[iterator->current_index / iterator->block_size] + 1) + iterator->element_size * block_offset;
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