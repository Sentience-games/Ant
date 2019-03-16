#pragma once

// TODO(soimn):
//
// - Ensure all functions behave on bad input and allocation failure
// - Align all allocations properly

#include "ant_shared.h"
#include "utils/memory_utils.h"
#include "utils/bit_utils.h"
#include "math/utils.h"
#include "math/exponentiation.h"

#define GPU_MEMORY_BLOCK_SIZE KILOBYTES(128)
#define GPU_MEMORY_BLOCK_SUB_BLOCK_COUNT 64

StaticAssert(GPU_MEMORY_BLOCK_SUB_BLOCK_COUNT % 8 == 0);

struct gpu_memory_block
{
    memory_index offset;
    
    u64 status;
    
    u8 largest_free;
    u8 largest_free_start;
    u8 free_left;
    u8 free_right;
};

struct gpu_memory_chunk
{
    u64 handle;
    
    memory_index block_size;
    memory_index block_alignment;
    
    gpu_memory_block* blocks;
    u16 block_count;
    
    // TODO(soimn): utilize these
    u16 first_free_block;
    u16 first_somewhat_free_block;
    
    u16 id;
    
    // TODO(soimn): use this to determine which chunk to use. Set in init and update in alloc
    memory_index largest_free;
};

struct gpu_memory_allocation
{
    memory_index offset;
    
    u16 chunk_id;
    
    // NOTE(soimn): this represents the starting block in multiblock allocations
    u16 block_id;
    
    u8 memory_start;
    u8 memory_size;
};

inline gpu_memory_allocation
GPUMemoryBlockAllocateSubBlocks_(gpu_memory_block* block, u16 sub_block_count, bool place_at_end = false)
{
    Assert(block);
    Assert(sub_block_count <= 64);
    
    gpu_memory_allocation result = {};
    
    u64 status = block->status;
    u64 mask   = (sub_block_count < 64 ? Pow2((u8) sub_block_count) - 1 : UINT64_MAX);
    
    if (place_at_end)
    {
        status = FlipAround(status);
    }
    
    u8 shift_count = 0;
    u8 index       = 0;
    
    for (;;)
    {
        index        = GetFirstSetBit(~status);
        status     >>= index;
        shift_count += index;
        
        Assert(index + shift_count < GPU_MEMORY_BLOCK_SUB_BLOCK_COUNT);
        
        if ((mask & status) == mask)
        {
            break;
        }
        
        else
        {
            status     >>= (u8) sub_block_count;
            shift_count += (u8) sub_block_count;
        }
    }
    
    if (!place_at_end)
    {
        block->status    |= (mask << shift_count);
        block->free_left  = MIN(block->free_left, shift_count);
        block->free_right = MIN(block->free_right, GPU_MEMORY_BLOCK_SUB_BLOCK_COUNT - (shift_count + (u8) sub_block_count));
        
        result.offset       = block->offset;
        result.memory_start = shift_count;
        result.memory_size  = (u8) sub_block_count;
    }
    
    else
    {
        u8 reverse_shift_count = (GPU_MEMORY_BLOCK_SUB_BLOCK_COUNT - (shift_count + (u8) sub_block_count));
        
        block->status |= (mask << reverse_shift_count);
        block->free_left = MIN(block->free_left, reverse_shift_count);
        block->free_right = MIN(block->free_right, shift_count);
        
        result.offset       = block->offset;
        result.memory_start = reverse_shift_count;
        result.memory_size  = (u8) sub_block_count;
    }
    
    
    // TODO(soimn): sanity check this
    if (block->largest_free_start == shift_count)
    {
        u64 not_status  = ~block->status;
        shift_count     = 0;
        u8 largest_free = 0;
        
        for (;;)
        {
            index        = GetFirstSetBit(~status);
            not_status >>= index;
            shift_count += index;
            
            if (not_status)
            {
                u8 current_free_length = 0;
                
                for (;; ++current_free_length)
                {
                    if ((not_status & 0x1) == 0)
                    {
                        break;
                    }
                }
                
                largest_free = MAX(largest_free, current_free_length);
            }
            
            else
            {
                break;
            }
        }
        
        block->largest_free = largest_free;
    }
    
    return result;
}

// TODO(soimn): make this thread safe
inline bool
GPUAllocateMemory(gpu_memory_chunk* chunk, memory_index size, memory_index alignment, gpu_memory_allocation* out_allocation)
{
    Assert(chunk && size && alignment && out_allocation);
    
    bool succeeded = false;
    
    memory_index total_size = size + AlignOffsetLarge(AlignLarge(NULL, chunk->block_alignment), alignment);
    
    f32 total_size_in_sb = Ceil((f32) total_size / (f32) chunk->block_size);
    
    // NOTE(soimn): sane allocations should not approach 8 TB of memory
    Assert(total_size_in_sb < (f32) UINT16_MAX);
    
    u16 sub_block_count = (u16) total_size_in_sb;
    u16 required_blocks = 1;
    
    {
        u16 sub_block_count_ = sub_block_count;
        
        while (sub_block_count_ > GPU_MEMORY_BLOCK_SUB_BLOCK_COUNT)
        {
            ++required_blocks;
            sub_block_count_ -= GPU_MEMORY_BLOCK_SUB_BLOCK_COUNT;
        }
    }
    
    if (required_blocks == 1)
    { 
        for (u16 i = 0; i < chunk->block_count; ++i)
        {
            gpu_memory_block* current_block = &chunk->blocks[i];
            
            if (current_block->largest_free >= sub_block_count)
            {
                *out_allocation = GPUMemoryBlockAllocateSubBlocks_(current_block, sub_block_count);
                
                out_allocation->chunk_id = chunk->id;
                out_allocation->block_id = i;
                
                succeeded = true;
            }
        }
    }
    
    else
    {
        for (u16 i = 0; i < chunk->block_count - 2; ++i)
        {
            gpu_memory_block* current_block = &chunk->blocks[i];
            gpu_memory_block* next_block    = &chunk->blocks[i + 1];
            
            if (current_block->free_right + next_block->free_left >= sub_block_count)
            {
                *out_allocation = GPUMemoryBlockAllocateSubBlocks_(current_block, current_block->free_right, true);
                
                u16 next_sub_block_count = sub_block_count - current_block->free_right;
                
                gpu_memory_allocation temp_allocation = GPUMemoryBlockAllocateSubBlocks_(next_block, next_sub_block_count);
                
                out_allocation->memory_size += temp_allocation.memory_size;
                
                out_allocation->chunk_id = chunk->id;
                out_allocation->block_id = i;
                
                succeeded = true;
            }
        }
    }
    
    return succeeded;
}