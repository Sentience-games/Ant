#pragma once

#include "ant_shared.h"
#include "utils/string.h"
#include "ant_memory.h"

#include <stdarg.h>

struct Error_Stream_Chunk
{
    String file_name;
    U32 line_nr;
    
    U8* contents;
    UMM size;
    
    Error_Stream_Chunk* next;
};

struct Error_Stream
{
    Memory_Arena arena;
    Error_Stream_Chunk* first_chunk;
    Error_Stream_Chunk* current_chunk;
    U32 chunk_count;
};

inline Error_Stream
BeginErrorStream(UMM block_size = KILOBYTES(4))
{
    Error_Stream stream = {};
    stream.arena.block_size = block_size;
    
    return stream;
}

inline void
EndErrorStream(Error_Stream* stream)
{
    ClearArena(&stream->arena);
}

inline void
PushError_(const char* file_name, U32 line_nr, Error_Stream* stream, const char* format_string, ...)
{
    va_list arg_list = {};
    va_start(arg_list, format_string);
    UMM required_size = FormatString(0, 0, format_string, arg_list);
    va_end(arg_list);
    
    Error_Stream_Chunk* new_chunk = PushStruct(&stream->arena, Error_Stream_Chunk);
    new_chunk->file_name = WrapCString(file_name);
    new_chunk->line_nr   = line_nr;
    
    new_chunk->size = required_size;
    new_chunk->contents = (U8*) PushSize(&stream->arena, required_size);
    
    if (stream->current_chunk)
    {
        stream->current_chunk->next = new_chunk;
    }
    
    else
    {
        stream->first_chunk = new_chunk;
    }
    
    stream->current_chunk = new_chunk;
    ++stream->chunk_count;
    
    va_start(arg_list, format_string);
    FormatString((char*) stream->current_chunk->contents, stream->current_chunk->size, format_string, arg_list);
    va_end(arg_list);
}

#define PushError(stream, format_string, ...) PushError_(__FILE__, __LINE__, stream, format_string, ## __VA_ARGS__)