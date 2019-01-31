#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#ifdef ANT_DEBUG
	
	#ifndef ANT_ASSERTION_ENABLED
	#define ANT_ASSERTION_ENABLED
	#endif

	#ifndef ANT_VULKAN_ENABLE_VALIDATION_LAYERS
	#define ANT_VULKAN_ENABLE_VALIDATION_LAYERS
	#endif

#endif

#include "ant_platform.h"

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

/// Vukan

#define VK_EXPORTED_FUNCTION(func) PFN_##func func
#define VK_GLOBAL_LEVEL_FUNCTION(func) PFN_##func func
#define VK_INSTANCE_LEVEL_FUNCTION(func) PFN_##func func
#define VK_DEVICE_LEVEL_FUNCTION(func) PFN_##func func

struct win32_vulkan_api
{
	#include "vulkan_platform_functions.inl"
};

global_variable win32_vulkan_api* VulkanAPI;

#undef VK_EXPORTED_FUNCTION
#undef VK_GLOBAL_LEVEL_FUNCTION
#undef VK_INSTANCE_LEVEL_FUNCTION
#undef VK_DEVICE_LEVEL_FUNCTION

/// Game

GAME_INIT_FUNCTION(GameInitStub)
{
}

GAME_UPDATE_FUNCTION(GameUpdateStub)
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
};

struct win32_game_info
{
	const char* name;
	const uint32 version;

	const char* dll_path;
	const char* loaded_dll_path;
};
