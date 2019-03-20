#pragma once

#ifdef ANT_DEBUG
#define ASSERTION_ENABLED
#endif

#include "ant_types.h"

/// PLATFORM API

/// Memory

#define PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(name) struct Memory_Block* name (Memory_Index size)
typedef PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(platform_allocate_Memory_Block_function);

#define PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(name) void name (struct Memory_Block* block)
typedef PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(platform_free_Memory_Block_function);


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


/// File API

struct platform_file_handle
{
	void* platform_data;
	B32 is_valid;
};

struct platform_file_info
{
	U64 timestamp;
	U64 file_size;
	char* base_name;
	void* platform_data;
    
	platform_file_info* next;
};

struct platform_file_group
{
	U32 file_count;
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

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(name) platform_file_group name (Enum32(PlatformFileTypeTag) file_type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(platform_get_all_files_of_type_begin_function);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(name) void name (platform_file_group* file_group)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(platform_get_all_files_of_type_end_function);

#define PLATFORM_OPEN_FILE_FUNCTION(name) platform_file_handle name (platform_file_info* file_info, U8 open_params)
typedef PLATFORM_OPEN_FILE_FUNCTION(platform_open_file_function);

#define PLATFORM_CLOSE_FILE_FUNCTION(name) void name (platform_file_handle* file_handle)
typedef PLATFORM_CLOSE_FILE_FUNCTION(platform_close_file_function);

#define PLATFORM_READ_FROM_FILE_FUNCTION(name) void name (platform_file_handle* handle, U64 offset, U64 size, void* dest)
typedef PLATFORM_READ_FROM_FILE_FUNCTION(platform_read_from_file_function);

#define PLATFORM_WRITE_TO_FILE_FUNCTION(name) void name (platform_file_handle* handle, U64 offset, U64 size, void* source)
typedef PLATFORM_WRITE_TO_FILE_FUNCTION(platform_write_to_file_function);

#define PLATFORM_FILE_IS_VALID(handle) ((handle)->is_valid)

/// Input

#define PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE 16

struct platform_key_code
{
    // NOTE(soimn): This is the key code for the keypress.
    //			  Alphanumeric key codes are encoded with the ascii character value.
    //			  Special ascii values often requires a modifier key to access, such as ':',
    //			  are encoded with the ascii character values, and the modifier keys are not registered.
    //
    //				<Enter>	 0x0D
    //				<Esc>	   0x1B
    //				<Backspace> 0x08
    //				<Space>	 0x20
    //				<Del>	   0x7F
    //				<Tab>	   0x09
	
    U8 code; 
	U8 modifiers;
	U8 Reserved_[2];
};

struct platform_game_input
{
	F32 frame_dt;
	
	U32 current_key_buffer_size;
	bool last_key_down;
	platform_key_code key_buffer[PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE];
    
};

struct platform_api_functions
{
    platform_allocate_Memory_Block_function* AllocateMemoryBlock;
    platform_free_Memory_Block_function* FreeMemoryBlock;
    
	platform_log_info_function* LogInfo;
	platform_log_error_function* LogError;
    
	platform_get_all_files_of_type_begin_function* GetAllFilesOfTypeBegin;
	platform_get_all_files_of_type_end_function* GetAllFilesOfTypeEnd;
	platform_open_file_function* OpenFile;
	platform_close_file_function* CloseFile;
	platform_read_from_file_function* ReadFromFile;
	platform_write_to_file_function* WriteToFile;
};

extern platform_api_functions* Platform;

struct game_memory
{
    struct game_state* state;
    
	platform_api_functions platform_api;
};

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (game_memory* memory, F32 delta_t, platform_game_input* old_input, platform_game_input* new_input)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);