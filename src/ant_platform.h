#pragma once

#include "ant_shared.h"
#include "ant_vfs.h"

/// Logging
enum LOG_OPTIONS
{
    Log_Info    = 0x1,
    Log_Warning = 0x2,
    Log_Error   = 0x4,
    Log_Fatal   = 0x8,
    
    Log_MessagePrompt = 0x20,
};

#define PLATFORM_LOG_FUNCTION(name) void name (U8 log_options, const char* message, ...)
typedef PLATFORM_LOG_FUNCTION(platform_log_function);

#define LOG_INFO(message, ...) Platform->Log(Log_Info, message, ##__VA_ARGS__)
#define LOG_WARNING(message, ...) Platform->Log(Log_Warning, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) Platform->Log(Log_Error | Log_MessagePrompt, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) Platform->Log(Log_Fatal | Log_MessagePrompt, message, ##__VA_ARGS__)

/// Memory
#define PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(name) struct Memory_Block* name (UMM block_size)
typedef PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(platform_allocate_memory_block_function);

#define PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(name) void name (struct Memory_Block* block)
typedef PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(platform_free_memory_block_function);

/// VFS
#define PLATFORM_RELOAD_VFS_FUNCTION(name) void name (VFS* vfs)
typedef PLATFORM_RELOAD_VFS_FUNCTION(platform_reload_vfs_function);

#define PLATFORM_GET_FILE_FUNCTION(name) File_Handle name (VFS* vfs, String path)
typedef PLATFORM_GET_FILE_FUNCTION(platform_get_file_function);

enum FILE_OPEN_FLAG
{
    File_OpenRead  = 0x1,
    File_OpenWrite = 0x2,
};

#define PLATFORM_OPEN_FILE_FUNCTION(name) bool name (VFS* vfs, File_Handle file_handle, Enum8(FILE_OPEN_FLAG) flags)
typedef PLATFORM_OPEN_FILE_FUNCTION(platform_open_file_function);

#define PLATFORM_CLOSE_FILE_FUNCTION(name) void name (VFS* vfs, File_Handle file_handle)
typedef PLATFORM_CLOSE_FILE_FUNCTION(platform_close_file_function);

#define PLATFORM_READ_FROM_FILE_FUNCTION(name) bool name (VFS* vfs, File_Handle file_handle, U32 offset, U32 size, void* memory, U32* bytes_read)
typedef PLATFORM_READ_FROM_FILE_FUNCTION(platform_read_from_file_function);

#define PLATFORM_WRITE_TO_FILE_FUNCTION(name) bool name (VFS* vfs, File_Handle file_handle, U32 offset, void* memory, U32 size, U32* bytes_written)
typedef PLATFORM_WRITE_TO_FILE_FUNCTION(platform_write_to_file_function);

#define PLATFORM_GET_FILE_SIZE_FUNCTION(name) U32 name (VFS* vfs, File_Handle file_handle)
typedef PLATFORM_GET_FILE_SIZE_FUNCTION(platform_get_file_size_function);

struct Platform_API
{
    platform_log_function* Log;
    
    platform_allocate_memory_block_function* AllocateMemoryBlock;
    platform_free_memory_block_function* FreeMemoryBlock;
    
    platform_reload_vfs_function* ReloadVFS;
    platform_get_file_function* GetFile;
    platform_open_file_function* OpenFile;
    platform_close_file_function* CloseFile;
    platform_read_from_file_function* ReadFromFile;
    platform_write_to_file_function* WriteToFile;
    platform_get_file_size_function* GetFileSize;
};

Platform_API* Platform;

struct Game_Memory
{
    struct Game_State* state;
    
    struct Memory_Arena* persistent_arena;
    struct Memory_Arena* frame_arena;
    
    VFS* vfs;
    Platform_API platform_api;
};

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (Game_Memory* game_memory)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);