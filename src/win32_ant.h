#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#undef near
#undef far

#ifdef ANT_DEBUG

#ifndef ASSERTION_ENABLED
#define ASSERTION_ENABLED
#endif

#ifndef ANT_ENABLE_HOT_RELOADING
#define ANT_ENABLE_HOT_RELOADING
#endif

#endif

#include "ant.h"
#include "ant_shared.h"
#include "ant_memory.h"

#include "utils/cstring.h"
#include "utils/string.h"

/// Renderer
PLATFORM_LOG_ERROR_FUNCTION(Win32LogError);
PLATFORM_LOG_INFO_FUNCTION(Win32LogInfo);

#include "renderer/renderer.h"
#include "renderer/renderer.cpp"

#ifndef _WIN64
#error "32-bit builds are not supported"
#endif

/// Debug

#define WIN32LOG_FATAL(message, ...) Win32LogError("Win32", true, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define WIN32LOG_ERROR(message, ...) Win32LogError("Win32", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)

#ifdef ANT_DEBUG
#define WIN32LOG_INFO(message, ...) Win32LogInfo("Win32", false, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define WIN32LOG_DEBUG(message, ...) Win32LogInfo("Win32", true, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#else
#define WIN32LOG_INFO(message, ...)
#define WIN32LOG_DEBUG(message, ...)
#endif

/// FILE API
struct Win32_File_Group
{
	Memory_Arena memory;
};

/// Game

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRenderStub)
{
}

struct Win32_Game_Code
{
	FILETIME timestamp;
	HMODULE module;
	bool is_valid;
    
	game_update_and_render_function* game_update_and_render_func;
};

struct Win32_Game_Info
{
	const char* name;
	const U32 version;
    
	const wchar_t* cwd;
	const U32 cwd_length;
    
	const wchar_t* dll_path;
	const wchar_t* loaded_dll_path;
};
