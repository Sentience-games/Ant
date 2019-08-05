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
    Log_Verbose       = 0x40,
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
#include "ant_keycodes.h"

enum GAME_BUTTONS
{
    Button_Test,
    
    GAME_BUTTON_COUNT
};

enum EDITOR_BUTTONS
{
    EditorButton_Test,
    
    EDITOR_BUTTON_COUNT
};

StaticAssert(GAME_BUTTON_COUNT <= U32_MAX);

#define GAME_BUTTON_HOLD_THRESHOLD MILLISECONDS(520)
struct Game_Button_State
{
    // NOTE(soimn): Normalized float depicting how pressed down the button is. Only relevant when dealing with 
    //              pressure sensitive input, like throttling and braking. If the button is digital, the value is 
    //              either 0.0 or 1.0.
    F32 actuation_amount;
    
    F32 hold_duration;
    U16 transition_count;
    B8 ended_down;
    B8 did_cross_hold_threshold;
};

struct Game_Controller_Input
{
    Game_Button_State buttons[GAME_BUTTON_COUNT];
    B32 is_gamepad;
    V2 stick_delta[2];
};

struct Game_Controller_Info
{
    U64 device_handle;
    Enum32(KEYCODE) gamepad_keymap[GAME_BUTTON_COUNT];
};

// NOTE(soimn): GAME_MAX_ACTIVE_CONTROLLER_COUNT does not include the default keyboard and mouse, keyboards and 
//              mice are not differentiated and are all mapped to the default controller (controller 0). If 
//              GAME_MAX_ACTIVE_CONTROLLER_COUNT is 0, 
//              GAME_SHOULD_PIPE_CONTROLLER_INPUT_AS_KEYBOARD_WHEN_NECESSARY is 1 and controller input is 
//              detected, the controller input will be piped through the default controller (controller 0) and 
//              mapped to the gamepad_keymap of the default controller.
#define GAME_MAX_ACTIVE_CONTROLLER_COUNT 0
#define GAME_SHOULD_PIPE_CONTROLLER_INPUT_AS_KEYBOARD_WHEN_NECESSARY 1
struct Platform_Game_Input
{
    F32 frame_dt;
    
    Game_Controller_Input active_controllers[GAME_MAX_ACTIVE_CONTROLLER_COUNT + 1];
    U32 active_controller_count;
    
    // TODO(soimn): Should this be integrated with the button states? It would be benefitial for expressiveness 
    //              and clarity when dealing with alternating input from a keyboard/mouse and a gamepad. However, 
    //              the value would then have to be somehow mapped to a normalized actuation amount. 
    V2 mouse_delta;
    
    V2 mouse_p;
    V2 normalized_mouse_p;
    I32 wheel_delta;
    
    
    B32 quit_requested;
    B32 editor_mode;
    
    Game_Button_State editor_buttons[EDITOR_BUTTON_COUNT];
};

inline U16
GetPressCount(Game_Button_State button)
{
    U16 half_press  = (button.hold_duration < 0.0f && button.hold_duration > -GAME_BUTTON_HOLD_THRESHOLD);
    U16 press_count = (button.transition_count + half_press) / 2;
    
    return press_count;
}

inline F32
GetHoldDuration(Game_Button_State button)
{
    return Abs(button.hold_duration);
}

inline bool
IsDown(Game_Button_State button)
{
    return (button.ended_down);
}

inline bool
IsHeld(Game_Button_State button)
{
    return (button.hold_duration >= GAME_BUTTON_HOLD_THRESHOLD);
}

inline bool
WasDown(Game_Button_State button)
{
    return (button.hold_duration != 0.0f);
}

inline bool
WasPressed(Game_Button_State button)
{
    return (GetPressCount(button) != 0);
}

inline bool
WasHeld(Game_Button_State button)
{
    return (button.hold_duration < 0.0f && button.hold_duration <= -GAME_BUTTON_HOLD_THRESHOLD);
}


inline bool
WasHeldLastFrame(Game_Button_State button)
{
    return (button.hold_duration >= GAME_BUTTON_HOLD_THRESHOLD && !button.did_cross_hold_threshold);
}

inline Game_Controller_Input
GetController(Platform_Game_Input* input, U32 controller_id)
{
    Assert(!controller_id || controller_id < input->active_controller_count);
    return input->active_controllers[controller_id];
}

inline Game_Button_State
GetButtonState(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Assert(button_id < GAME_BUTTON_COUNT);
    return controller.buttons[button_id];
}

inline U16
GetPressCount(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return GetPressCount(button);
}

inline F32
GetHoldDuration(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return GetHoldDuration(button);
}

inline bool
IsDown(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return IsDown(button);
}

inline bool
IsHeld(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return IsHeld(button);
}

inline bool
WasDown(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return WasDown(button);
}

inline bool
WasPressed(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return (GetPressCount(button) != 0);
}

inline bool
WasHeld(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return WasHeld(button);
}


inline bool
WasHeldLastFrame(Game_Controller_Input controller, Enum32(GAME_BUTTONS) button_id)
{
    Game_Button_State button = GetButtonState(controller, button_id);
    return WasHeldLastFrame(button);
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
    
    Game_Controller_Info controller_infos[GAME_MAX_ACTIVE_CONTROLLER_COUNT + 1];
    U32 active_controller_count;
    
    Enum32(KEYCODE) keyboard_game_keymap[GAME_BUTTON_COUNT];
    Enum32(KEYCODE) keyboard_editor_keymap[EDITOR_BUTTON_COUNT];
    
    Platform_API platform_api;
};

#define GAME_INIT_FUNCTION(name) void name (Game_Memory* game_memory)
typedef GAME_INIT_FUNCTION(game_init_function);

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (Game_Memory* game_memory, Platform_Game_Input* input)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);