#pragma once

#include "ant_shared.h"

#include "ant_vfs.h"

#include "math/vector.h"

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

/// Input
#include "ant_keys.h"

struct Game_Button_State
{
    // NOTE(soimn): This signifies the amount of time which has passed since the button was pressed down and is 
    //              positive as long as the button is pressed, and turns negative when released. This is done to 
    //              destinguish between a previous hold/press and a newly detected one.
    F32 hold_duration;
    F32 last_hold_duration;
    
    // NOTE(soimn): This signifies whether or not the button was down when the input recording for that frame 
    //              ended
    B32 ended_down;
};

#define GAME_MAX_CONTROLLER_COUNT 1
#define GAME_BUTTON_COUNT 1

StaticAssert(GAME_MAX_CONTROLLER_COUNT < U32_MAX);

struct Game_Controller_Input
{
    union
    {
        Game_Button_State buttons[GAME_BUTTON_COUNT];
        
        struct
        {
            Game_Button_State test_button;
        };
    };
    
    union
    {
        V2 stick_delta;
        V2 mouse_delta
    };
    
    V2 normalized_mouse_p;
    I32 mouse_wheel_delta;
    
    bool is_analog;
};

struct Game_Controller_Info
{
    U64 keymap[GAME_BUTTON_COUNT];
    U64 device_handle;
    U64 additional_device_handle;
    bool was_remapped;
};

struct Platform_Game_Input
{
    F32 frame_dt;
    
    Game_Controller_Input controllers[GAME_MAX_CONTROLLER_COUNT];
};

// TODO(soimn): Move these to shared globals
// NOTE(soimn): These values are based on limited testing and should be adjusted further
#define GAME_BUTTON_HOLD_THRESHOLD MILLISECONDS(520)

// TODO(soimn): Should input responsiveness be sacrificed in order to allow double-tapping?
// #define GAME_BUTTON_DOUBLE_TAP_THRESHOLD MILLISECONDS(160)

// IMPORTANT
// NOTE(soimn): The current system discards all sub-frame keypresses, this means the shortest keypress possible is 
//              one where the press starts at the frame boundary of the last frame and ends at the beginning of 
//              the next. This might change in the future, however in the current state, it seems as if there 
//              is no sane explanation for supporting sub-frame keypresses, since a 40ms keypress (sub-frame 
//              at 10 FPS) requires a person to mash one key nearly as fast as the fastest recorded typing speed, 
//              as of 2019, at 216 WPM, or 18 characters per second, which amounts to ~2 chars in one 10 FPS 
//              frame. At 10 FPS the game is nowhere near playable, so this granularity of input seems ridiculous.
//              However, a case could be made that this statement only limits the functionality of the engine and 
//              rythm games, like Osu!, would strongly benefit from a more accurate and robust system, however the 
//              granularity and reliability expected from such a game's input requires a system tailored to that 
//              specific game and it's limits. Therefore, it seems, as of now, that sub-frame keypress detection 
//              is overkill and completely unnecessary.

// IsDown - Checks if the button is down
inline bool
IsDown(Game_Button_State button)
{
    return (button.ended_down);
}

// IsReleased - checks if the button was released this frame
inline bool
IsReleased(Game_Button_State button)
{
    return (button.hold_duration < 0.0f);
}

// IsHeld - checks if the button is currently held
inline bool
IsHeld(Game_Button_State button)
{
    return (button.hold_duration >= GAME_BUTTON_HOLD_THRESHOLD);
}

// WasDown - Reports whether or not the button was down last frame
inline bool
WasDown(Game_Button_State button)
{
    return (button.hold_duration != 0.0f);
}

// WasReleased - reports whether or not the button was released last frame
inline bool
WasReleased(Game_Button_State button)
{
    return (button.old_hold_duration < 0.0f);
}

// WasHeld - Reports whether or not the button was held the previous frame
inline bool
WasHeld(Game_Button_State button)
{
    return (button.old_hold_duration >= GAME_BUTTON_HOLD_THRESHOLD);
}

// PressedAndReleased - Checks if the button has been released and the hold duration is shorter than the hold 
//                      threshold
inline bool
PressedAndReleased(Game_Button_State button)
{
    return (IsReleased(button) && button.hold_duration * -1.0f < GAME_BUTTON_HOLD_THRESHOLD);
}

// HeldAndReleased - Checks if the button has been released and the hold duration is longer than the hold 
//                   threshold
inline bool
HeldAndReleased(Game_Button_State button)
{
    return (IsReleased(button) && button.hold_duration * -1.0f >= GAME_BUTTON_HOLD_THRESHOLD);
}

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
    
    Game_Controller_Info controller_infos[GAME_MAX_CONTROLLER_COUNT];
    U32 active_controller_count;
    
    Platform_API platform_api;
};

#define GAME_INIT_FUNCTION(name) void name (Game_Memory* game_memory)
typedef GAME_INIT_FUNCTION(game_init_function);

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (Game_Memory* game_memory, Platform_Game_Input* input)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);