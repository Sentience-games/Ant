#pragma once

#include "ant_shared.h"
#include "utils/string.h"
#include "ant_memory.h"

#include <stdarg.h>

struct Error_Stream_Chunk
{
    String file_name;
    U32 line_nr;
    
    String contents;
    
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
PushError_(const char* file_name, U32 line_nr, Error_Stream* stream, const char* format, ...)
{
    UMM format_length = StrLength(format);
    String string_format = {format_length, (U8*) format};
    
    va_list arg_list = {};
    va_start(arg_list, format);
    UMM required_size = FormatString(0, 0, string_format, arg_list);
    va_end(arg_list);
    
    Error_Stream_Chunk* new_chunk = PushStruct(&stream->arena, Error_Stream_Chunk);
    new_chunk->file_name = WrapCString(file_name);
    new_chunk->line_nr   = line_nr;
    
    new_chunk->contents.size = required_size;
    new_chunk->contents.data = (U8*) PushSize(&stream->arena, required_size);
    
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
    
    va_start(arg_list, format);
    FormatString((char*) stream->current_chunk->contents.data, stream->current_chunk->contents.size, string_format, arg_list);
    va_end(arg_list);
}

#define PushError(stream, format, ...) PushError_(__FILE__, __LINE__, stream, format, ## __VA_ARGS__)


inline void
LogErrorStream(Error_Stream* stream)
{
    Error_Stream_Chunk* scan = stream->first_chunk;
    for (U32 i = 0; i < stream->chunk_count; ++i)
    {
        Platform->Log(scan->contents);
        
        scan = scan->next;
    }
}