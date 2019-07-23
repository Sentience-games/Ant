#pragma once

enum PLATFORM_LOG_OPTIONS
{
    PlatformLog_Info    = 0x1,
    PlatformLog_Warning = 0x2,
    PlatformLog_Error   = 0x4,
    PlatformLog_Fatal   = 0x8,
    
    PlatformLog_MessagePrompt = 0x20,
};

/// Logging
#define PLATFORM_LOG_FUNCTION(name) void name (U8 log_options, const char* message, ...)
typedef PLATFORM_LOG_FUNCTION(platform_log_function);

#define LOG_INFO(message, ...) Platform->Log(PlatformLog_Info, message, ##__VA_ARGS__)
#define LOG_WARNING(message, ...) Platform->Log(PlatformLog_Warning, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) Platform->Log(PlatformLog_Error | PlatformLog_MessagePrompt, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) Platform->Log(PlatformLog_Fatal | PlatformLog_MessagePrompt, message, ##__VA_ARGS__)

/// Memory
#define PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(name) struct Memory_Block* name (UMM block_size)
typedef PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(platform_allocate_memory_block_function);

#define PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(name) void name (struct Memory_Block* block)
typedef PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(platform_free_memory_block_function);

/// File System Interaction

struct Platform_API
{
    platform_log_function* Log;
    
    platform_allocate_memory_block_function* AllocateMemoryBlock;
    platform_free_memory_block_function* FreeMemoryBlock;
};

Platform_API* Platform;

struct Game_Memory
{
    struct Game_State* state;
    
    struct Memory_Arena* persistent_arena;
    struct Memory_Arena* frame_arena;
    
    Platform_API platform_api;
};

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (Game_Memory* game_memory)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);