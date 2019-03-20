#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#ifdef ANT_DEBUG

#ifndef ASSERTION_ENABLED
#define ASSERTION_ENABLED
#endif

#ifndef ANT_ENABLE_HOT_RELOADING
#define ANT_ENABLE_HOT_RELOADING
#endif

#endif

#include "ant.h"

#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "utils/assert.h"
#include "utils/cstring.h"

// TODO(soimn): move these to the build arguments
#define APPLICATION_NAME "ant"
#define APPLICATION_VERSION 0
#define DEFAULT_TARGET_FPS 60

#define WIN32_EXPORT extern "C" __declspec(dllexport)

#ifndef _WIN64
#error "32-bit builds are not supported"
#endif

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

/// FILE API
struct win32_platform_file_group
{
	Memory_Arena memory;
};

/// Game

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRenderStub)
{
}

struct win32_game_code
{
	FILETIME timestamp;
	HMODULE module;
	bool is_valid;
    
	game_update_and_render_function* game_update_and_render_func;
};

struct win32_game_info
{
	const char* name;
	const U32 version;
    
	const wchar_t* cwd;
	const U32 cwd_length;
    
	const wchar_t* dll_path;
	const wchar_t* loaded_dll_path;
};
