#pragma once

#ifdef ANT_DEBUG
#define ASSERTION_ENABLED
#endif

#include "ant_shared.h"

#include "utils/assert.h"
#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "utils/cstring.h"

#include "math/vector.h"
#include "math/interpolation.h"

#include "assets/assets.h"

#include "vulkan/vulkan_header.h"

global_variable vulkan_renderer_state* Vulkan;

#include "immediate/immediate.h"

#ifdef ANT_PLATFORM_WINDOWS
#define EXPORT extern "C" __declspec(dllexport)
#endif

/// Logging

#define PLATFORM_LOG_INFO_FUNCTION(name) void name (const char* module, bool is_debug, const char* function_name,\
unsigned int line_nr, const char* message)
typedef PLATFORM_LOG_INFO_FUNCTION(platform_log_info_function);
#define PLATFORM_LOG_ERROR_FUNCTION(name) void name (const char* module, bool is_fatal, const char* function_name,\
unsigned int line_nr, const char* message)
typedef PLATFORM_LOG_ERROR_FUNCTION(platform_log_error_function);

#define LOG_FATAL(message) Platform->LogError("GAME", true, __FUNCTION__, __LINE__, message)
#define LOG_ERROR(message) Platform->LogError("GAME", false, __FUNCTION__, __LINE__, message)

#ifdef ANT_DEBUG
#define LOG_INFO(message) Platform->LogInfo("GAME", false, __FUNCTION__, __LINE__, message)
#define LOG_DEBUG(message) Platform->LogInfo("GAME", true, __FUNCTION__, __LINE__, message)
#else
#define LOG_INFO(message)
#define LOG_DEBUG(message)
#endif

/// PLATFORM API

/// File API

struct platform_file_handle
{
	void* platform_data;
	b32 is_valid;
};

struct platform_file_info
{
	u64 timestamp;
	u64 file_size;
	char* base_name;
	void* platform_data;
    
	platform_file_info* next;
};

struct platform_file_group
{
	u32 file_count;
	platform_file_info* first_file_info;
	void* platform_data;
};

enum PlatformFileTypeTag
{
	PlatformFileType_AssetFile,
	PlatformFileType_SaveFile,
	PlatformFileType_DDS,
	PlatformFileType_WAV,
	PlatformFileType_AAMF,
    
	// NOTE(soimn): this is mostly for debugging
	PlatformFileType_ShaderFile,
    
	PlatformFileType_TagCount
};

enum PlatformOpenFileOpenFlags
{
	OpenFile_Read  = 0x1,
	OpenFile_Write = 0x2
};

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(name) platform_file_group name (enum32(PlatformFileTypeTag) file_type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(platform_get_all_files_of_type_begin_function);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(name) void name (platform_file_group* file_group)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(platform_get_all_files_of_type_end_function);

#define PLATFORM_OPEN_FILE_FUNCTION(name) platform_file_handle name (platform_file_info* file_info, u8 open_params)
typedef PLATFORM_OPEN_FILE_FUNCTION(platform_open_file_function);

#define PLATFORM_CLOSE_FILE_FUNCTION(name) void name (platform_file_handle* file_handle)
typedef PLATFORM_CLOSE_FILE_FUNCTION(platform_close_file_function);

#define PLATFORM_READ_FROM_FILE_FUNCTION(name) void name (platform_file_handle* handle, u64 offset, u64 size, void* dest)
typedef PLATFORM_READ_FROM_FILE_FUNCTION(platform_read_from_file_function);

#define PLATFORM_WRITE_TO_FILE_FUNCTION(name) void name (platform_file_handle* handle, u64 offset, u64 size, void* source)
typedef PLATFORM_WRITE_TO_FILE_FUNCTION(platform_write_to_file_function);

#define PLATFORM_FILE_IS_VALID(handle) ((handle)->is_valid)

/// Input

// TODO(soimn):
// typedef struct platform_game_input_button
// {
// 	b32 ended_down;
// 	u32 half_transition_count;
// 
// 	// NOTE(soimn): this value is far from accurate, since frame time may differ
// 	f32 ms_since_last_transition;
// } platform_game_input_button;
// 
// typedef struct platform_game_input_axial
// {
// 	v2 value;
// } platform_game_input_axial;

#define PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE 16

typedef struct platform_key_code
{
	alignas(MEMORY_32BIT_ALIGNED)
        // NOTE(soimn): This is the key code for the keypress.
        //				Alphanumeric key codes are encoded with the ascii character value.
        //				Special ascii values often requires a modifier key to access, such as ':',
        //				are encoded with the ascii character values, and the modifier keys are not registered.
        //
        //				<Enter>		0x0D
        //				<Esc>		0x1B
        //				<Backspace> 0x08
        //				<Space>		0x20
        //				<Del>		0x7F
        //				<Tab>		0x09
	
        u8 code; 
	u8 modifiers;
	u8 Reserved_[2];
} platform_key_code;

typedef struct platform_game_input
{
	f32 frame_dt;
	
	/// UI INTERACTION ///
	
	struct mouse
	{
		v2 position;
        
		bool right, left, middle;
	} mouse;
    
	bool tab;
	bool enter;
    
	u32 current_key_buffer_size;
	bool last_key_down;
	platform_key_code key_buffer[PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE];
    
} platform_game_input;

typedef struct platform_api_functions
{
	platform_log_info_function* LogInfo;
	platform_log_error_function* LogError;
    
	platform_get_all_files_of_type_begin_function* GetAllFilesOfTypeBegin;
	platform_get_all_files_of_type_end_function* GetAllFilesOfTypeEnd;
	platform_open_file_function* OpenFile;
	platform_close_file_function* CloseFile;
	platform_read_from_file_function* ReadFromFile;
	platform_write_to_file_function* WriteToFile;
} platform_api_functions;

typedef struct game_memory
{
	bool is_initialized;
    
	platform_api_functions platform_api;
    
	vulkan_renderer_state vulkan_state;
    
	platform_game_input new_input;
	platform_game_input old_input;
    
	default_memory_arena_allocation_routines default_allocation_routines;
	memory_arena persistent_arena;
	memory_arena frame_temp_arena;
    
	memory_arena debug_arena;
} game_memory;

#define GAME_INIT_FUNCTION(name) bool name (game_memory* memory)
typedef GAME_INIT_FUNCTION(game_init_function);

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (game_memory* memory, float delta_t)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);

#define GAME_CLEANUP_FUNCTION(name) void name (game_memory* memory)
typedef GAME_CLEANUP_FUNCTION(game_cleanup_function);
