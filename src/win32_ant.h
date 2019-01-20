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
#ifdef ANT_CONSOLE_ENABLED
#define WIN32LOG_FATAL(message)\
	Win32Log(message),\
	MessageBoxA(NULL, message, NULL, MB_OK | MB_ICONERROR),\
	running = false

#define WIN32LOG_ERROR(message)\
	Win32Log(message),\
	MessageBoxA(NULL, message, NULL, MB_OK | MB_ICONERROR)
#else
#define WIN32LOG_FATAL(message) MessageBoxA(NULL, message, NULL, MB_OK | MB_ICONERROR), running = false
#define WIN32LOG_ERROR(message) MessageBoxA(NULL, message, NULL, MB_OK | MB_ICONERROR)
#endif

#if defined(ANT_CONSOLE_ENABLED) && defined(ANT_DEBUG)
#define WIN32LOG_DEBUG(message) Win32Log(message)
#else
#define WIN32LOG_DEBUG(message) Win32Log(message)
#endif

/// Vulkan
#define MAX_VULKAN_APP_NAME 60 


struct vulkan_application
{
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkSurfaceKHR surface;

	const char* layers[16];
	const char* extensions[16];

	uint32 layer_count;
	uint32 extension_count;

	VkDebugUtilsMessengerEXT debug_messenger;

	bool has_compute_queue;
	VkQueue gfx_queue;
	VkQueue transfer_queue;
	VkQueue compute_queue;
};
