#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#ifdef ANT_DEBUG
	
	#ifndef ANT_ASSERTION_ENABLED
	#define ANT_ASSERTION_ENABLED
	#endif

	#ifndef ANT_VULKAN_ENABLE_VALIDATION_LAYERS
	#define ANT_VULKAN_ENABLE_VALIDATION_LAYERS
	#endif

	#ifndef ANT_ENABLE_HOT_RELOADING
	#define	ANT_ENABLE_HOT_RELOADING
	#endif

#endif

#include "ant_platform.h"

#ifdef ANT_DEBUG
#define DEBUG_MODE
#endif
#define PLATFORM_WINDOWS
#include "renderer/renderer.h"
#undef PLATFORM_WINDOWS
#ifdef ANT_DEBUG
#undef DEBUG_MODE
#endif

#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "utils/assert.h"
#include "utils/cstring.h"

// TODO(soimn): move these to the build arguments
#define APPLICATION_NAME "ant"
#define APPLICATION_VERSION 0

#define WIN32_EXPORT extern "C" __declspec(dllexport)

/// Debug

#define WIN32LOG_FATAL(message) Win32LogError("WIN32", true, __FUNCTION__, __LINE__, message)
#define WIN32LOG_ERROR(message) Win32LogError("WIN32", false, __FUNCTION__, __LINE__, message)

#ifdef ANT_DEBUG
#define WIN32LOG_INFO(message) Win32LogInfo("WIN32", false, __FUNCTION__, __LINE__, message)
#define WIN32LOG_DEBUG(message) Win32LogInfo("WIN32", true, __FUNCTION__, __LINE__, message)
#else
#define WIN32LOG_INFO(message)
#define WIN32LOG_DEBUG(message)
#endif

/// Utility
// NOTE(soimn): This is only used for readability, and should only be used in the setup stage of the WinMain function
#define CLAMPED_REMAINING_MEMORY(ptr, max) (uint32) CLAMP(0, memory.persistent_size - ((ptr) - (uint8*)memory.persistent_memory), (max))

/// Game

GAME_INIT_FUNCTION(GameInitStub)
{
	return 0;
}

GAME_UPDATE_FUNCTION(GameUpdateStub)
{
}

GAME_CLEANUP_FUNCTION(GameCleanupStub)
{
}

#define ANT_MAX_GAME_NAME_LENGTH 128


struct win32_game_code
{
	FILETIME timestamp;
	HMODULE module;
	bool is_valid;

	game_init_function* game_init_func;
	game_update_function* game_update_func;
	game_cleanup_function* game_cleanup_func;
};

struct win32_game_info
{
	const char* name;
	const uint32 version;

	const char* root_dir;
	const char* resource_dir;

	const char* dll_path;
	const char* loaded_dll_path;
};
