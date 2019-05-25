#pragma once

#ifdef ANT_DEBUG
#define ASSERTION_ENABLED
#endif

#include "ant_shared.h"
#include "renderer/renderer.h"

/// PLATFORM API

/// Memory

#define PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(name) struct Memory_Block* name (UMM size)
typedef PLATFORM_ALLOCATE_MEMORY_BLOCK_FUNCTION(platform_allocate_Memory_Block_function);

#define PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(name) void name (struct Memory_Block* block)
typedef PLATFORM_FREE_MEMORY_BLOCK_FUNCTION(platform_free_Memory_Block_function);


/// Logging

#define PLATFORM_LOG_INFO_FUNCTION(name) void name (const char* module, bool is_debug, const char* function_name,\
unsigned int line_nr, const char* format, ...)
typedef PLATFORM_LOG_INFO_FUNCTION(platform_log_info_function);
#define PLATFORM_LOG_ERROR_FUNCTION(name) void name (const char* module, bool is_fatal, const char* function_name,\
unsigned int line_nr, const char* format, ...)
typedef PLATFORM_LOG_ERROR_FUNCTION(platform_log_error_function);

#define LOG_FATAL(message, ...) Platform->LogError("Game", true, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) Platform->LogError("Game", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)

#ifdef ANT_DEBUG
#define LOG_INFO(message, ...) Platform->LogInfo("Game", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_DEBUG(message, ...) Platform->LogInfo("Game", true, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#else
#define LOG_INFO(message, ...)
#define LOG_DEBUG(message, ...)
#endif


/// File API

struct Platform_File_Handle
{
	void* platform_data;
	B32 is_valid;
};

struct Platform_File_Info
{
	U64 timestamp;
	U64 file_size;
	char* base_name;
	void* platform_data;
    
	Platform_File_Info* next;
};

struct Platform_File_Group
{
	Platform_File_Info* first_file_info;
	void* platform_data;
    U32 file_count;
};

enum PLATFORM_FILE_TYPE
{
	Platform_AssetFile,
    
	PLATFORM_FILE_TYPE_COUNT
};

enum PLATFORM_OPEN_FILE_FLAGS
{
	Platform_OpenRead  = 0x1,
	Platform_OpenWrite = 0x2
};

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(name)\
Platform_File_Group  name (Enum32(PLATFORM_FILE_TYPE) file_type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(platform_get_all_files_of_type_begin_function);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(name) void name (Platform_File_Group* file_group)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(platform_get_all_files_of_type_end_function);

#define PLATFORM_OPEN_FILE_UTF8_FUNCTION(name)\
Platform_File_Handle name (Memory_Arena* temporary_memory, String file_path, Flag8(PLATFORM_OPEN_FILE_FLAGS) open_flags)
typedef PLATFORM_OPEN_FILE_UTF8_FUNCTION(platform_open_file_utf8_function);

#define PLATFORM_OPEN_FILE_FUNCTION(name)\
Platform_File_Handle name (Platform_File_Info* file_info, Flag8(PLATFORM_OPEN_FILE_FLAGS) open_flags)
typedef PLATFORM_OPEN_FILE_FUNCTION(platform_open_file_function);

#define PLATFORM_CLOSE_FILE_FUNCTION(name) void name (Platform_File_Handle* file_handle)
typedef PLATFORM_CLOSE_FILE_FUNCTION(platform_close_file_function);

#define PLATFORM_READ_FROM_FILE_FUNCTION(name) void name (Platform_File_Handle* handle, U64 offset, U64 size, void* dest)
typedef PLATFORM_READ_FROM_FILE_FUNCTION(platform_read_from_file_function);

#define PLATFORM_WRITE_TO_FILE_FUNCTION(name) void name (Platform_File_Handle* handle, U64 offset, U64 size, void* source)
typedef PLATFORM_WRITE_TO_FILE_FUNCTION(platform_write_to_file_function);

#define PLATFORM_FILE_IS_VALID(handle) ((handle)->is_valid)

/// Input

#define PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE 16

struct Platform_Key_Code
{
    // NOTE(soimn): This is the key code for the keypress.
    //			  Alphanumeric key codes are encoded with the ascii character value.
    //			  Special ascii values often requires a modifier key to access, such as ':',
    //			  and are encoded with the ascii character values, and the modifier keys are not registered.
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

struct Platform_Game_Input
{
	F32 frame_dt;
	
	U32 current_key_buffer_size;
	bool last_key_down;
    Platform_Key_Code key_buffer[PLATFORM_GAME_INPUT_KEYBUFFER_MAX_SIZE];
    
};

struct Platform_API_Functions
{
    platform_allocate_Memory_Block_function* AllocateMemoryBlock;
    platform_free_Memory_Block_function* FreeMemoryBlock;
    
	platform_log_info_function* LogInfo;
	platform_log_error_function* LogError;
    
	platform_get_all_files_of_type_begin_function* GetAllFilesOfTypeBegin;
	platform_get_all_files_of_type_end_function* GetAllFilesOfTypeEnd;
    platform_open_file_utf8_function* OpenFileUTF8;
	platform_open_file_function* OpenFile;
	platform_close_file_function* CloseFile;
	platform_read_from_file_function* ReadFromFile;
	platform_write_to_file_function* WriteToFile;
    
    renderer_prepare_frame_function* PrepareFrame;
    renderer_push_mesh_function* PushMesh;
    renderer_present_frame_function* PresentFrame;
    
    renderer_create_prepping_batch_function* CreatePreppingBatch;
    renderer_prep_render_batch_function* PrepBatch;
    renderer_render_batch_function* RenderBatch;
    renderer_clean_batch_function* CleanBatch;
    
    renderer_create_texture_function* CreateTexture;
    renderer_delete_texture_function* DeleteTexture;
};

extern Platform_API_Functions* Platform;

struct Game_Memory
{
    bool is_initialized;
    
    struct Game_State* state;
    
    Platform_API_Functions platform_api;
};

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (Game_Memory* memory, Platform_Game_Input* old_input, Platform_Game_Input* new_input)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);