#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <new>

#include "ant.h"

#include "utils/utility_defines.h"
#include "utils/assert.h"
#include "utils/cstring.h"

/// Debug

enum DEBUG_LOG_CATEGORY
{
	DEBUG_LOG_ERROR,
	DEBUG_LOG_WARNING,
	DEBUG_LOG_INFO,
	DEBUG_LOG_DEBUG,

	DEBUG_LOG_CATEGORY_COUNT
};

global_variable
const char* debug_log_category_table[DEBUG_LOG_CATEGORY_COUNT] = {"ERROR",
																  "WARNING",
																  "INFO",
																  "DEBUG"};

#ifdef ANT_DEBUG
	
	#ifndef ANT_CONSOLE_ENABLED
	#define ANT_CONSOLE_ENABLED
	#endif
	
	#ifndef ANT_ASSERTION_ENABLED
	#define ANT_ASSERTION_ENABLED
	#endif

	#ifndef ANT_VULKAN_ENABLE_VALIDATION_LAYERS
	#define ANT_VULKAN_ENABLE_VALIDATION_LAYERS
	#endif

#endif

// TODO(soimn): engineer debugging systems such that the clean-up functions in WinMain are called at engine failure

#define WIN32LOG_FATAL(message) MessageBoxA(NULL, message, NULL, MB_OK | MB_ICONERROR), running = false

#ifdef ANT_CONSOLE_ENABLED
#define WIN32LOG_ERROR(message)\
	Win32DebugLog(DEBUG_LOG_ERROR, __FUNCTION__, __LINE__, message)
#else
#define WIN32LOG_ERROR(message) MessageBoxA(NULL, message, NULL, MB_OK | MB_ICONERROR)
#endif

#if defined(ANT_CONSOLE_ENABLED) && defined(ANT_DEBUG)
#define WIN32LOG_DEBUG(message) Win32DebugLog(DEBUG_LOG_DEBUG, __FUNCTION__, __LINE__, message)
#else
#define WIN32LOG_DEBUG(message)
#endif

/// Vulkan
#define MAX_VULKAN_APP_NAME 60
#define ANT_VULKAN_INSTANCE_EXTENSION_COUNT_LIMIT 16
#define ANT_VULKAN_INSTANCE_LAYER_COUNT_LIMIT     16
#define ANT_VULKAN_DEVICE_EXTENSION_COUNT_LIMIT   16

struct win32_vulkan_application
{
	bool initialized;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;
	VkImage* swapchain_images;
	uint32 swapchain_image_count;
	VkImageView* swapchain_image_views;
	uint32 swapchain_image_view_count;
	VkFormat swapchain_image_format;
	VkExtent2D swapchain_extent;

	const char* layers[ANT_VULKAN_INSTANCE_LAYER_COUNT_LIMIT];
	const char* extensions[ANT_VULKAN_INSTANCE_EXTENSION_COUNT_LIMIT];

	uint32 layer_count;
	uint32 extension_count;

	const char* device_extensions[ANT_VULKAN_DEVICE_EXTENSION_COUNT_LIMIT];
	uint32 device_extension_count;

	VkDebugUtilsMessengerEXT debug_messenger;

	bool has_compute_queue;
	bool has_separate_present_queue;
	VkQueue gfx_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;
	VkQueue compute_queue;
};

/// Game

typedef GAME_INIT_FUNCTION(game_init_function);
GAME_INIT_FUNCTION(GameInitStub)
{
}

typedef GAME_UPDATE_FUNCTION(game_update_function);
GAME_UPDATE_FUNCTION(GameUpdateStub)
{
}

#define ANT_MAX_GAME_NAME_LENGTH 128

struct win32_game_info
{
	const char* name;
	const uint32 version;

	const char* dll_path;

	HANDLE dll_handle;
	const char* loaded_dll_path;
};

struct win32_game_code
{
	FILETIME timestamp;
	HMODULE module;
	bool is_valid;

	game_init_function* game_init_func;
	game_update_function* game_update_func;
};
