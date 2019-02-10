#pragma once

#ifdef ANT_PLATFORM_WINDOWS
#define PLATFORM_WINDOWS
#endif

#ifdef ANT_DEBUG
#define DEBUG_MODE
#endif

#include "renderer/renderer.h"

#ifdef ANT_DEBUG
#undef DEBUG_MODE
#endif

#ifdef ANT_PLATFORM_WINDOWS
#undef PLATFORM_WINDOWS
#endif

#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "utils/fixed_int.h"

#ifdef ANT_PLATFORM_WINDOWS
#define EXPORT extern "C" __declspec(dllexport)
#endif

/// Logging

#define PLATFORM_LOG_INFO(name) void name (const char* module, bool is_debug, const char* function_name, unsigned int line_nr, const char* message)
typedef PLATFORM_LOG_INFO(platform_log_info);
#define PLATFORM_LOG_ERROR(name) void name (const char* module, bool is_fatal, const char* function_name, unsigned int line_nr, const char* message)
typedef PLATFORM_LOG_ERROR(platform_log_error);
	
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

typedef uint64 platform_file_size;
typedef const char* platform_file_path;

struct platform_file_handle
{
	void* data;
	platform_file_size size;
	uint8 adjustment;
};

#define DEBUG_READ_FILE(name) platform_file_handle name (platform_file_path path, uint8 data_alignment)
typedef DEBUG_READ_FILE(debug_read_file);

#define DEBUG_WRITE_FILE(name) bool name (platform_file_path path, void* memory, uint32 memory_size)
typedef DEBUG_WRITE_FILE(debug_write_file);

#define DEBUG_FREE_FILE_MEMORY(name) void name (platform_file_handle file_handle)
typedef DEBUG_FREE_FILE_MEMORY(debug_free_file_memory);

typedef struct platform_renderer_api
{
	renderer_state* RendererState;
	vulkan_api* VulkanAPI;

	renderer_create_swapchain_function* CreateSwapchain;
	renderer_create_swapchain_images_function* CreateSwapchainImages;
	renderer_create_render_pass_function* CreateRenderPass;
	renderer_create_pipeline_layout_function* CreatePipelineLayout;
	renderer_create_shader_module_function* CreateShaderModule;
	renderer_create_shader_stage_infos_function* CreateShaderStageInfos;
	renderer_create_graphics_pipelines_function* CreateGraphicsPipelines;
	renderer_create_framebuffer_function* CreateFramebuffer;
	renderer_create_command_pool_function* CreateCommandPool;
	renderer_create_semaphore_function* CreateSemaphore;
	renderer_get_optimal_swapchain_extent_function* GetOptimalSwapchainExtent;
	renderer_get_optimal_swapchain_surface_format_function* GetOptimalSwapchainSurfaceFormat;
	renderer_get_optimal_swapchain_present_mode_function* GetOptimalSwapchainPresentMode;
} platform_renderer_functions;

typedef struct platform_api_functions
{
	platform_log_info* LogInfo;
	platform_log_error* LogError;

	debug_read_file* DebugReadFile;
	debug_write_file* DebugWriteFile;
	debug_free_file_memory* DebugFreeFileMemory;

	platform_renderer_api RendererAPI;

} platform_api_functions;

typedef struct game_memory
{
	bool is_initialized;

	void* persistent_memory;
	void* transient_memory;
	uint64 persistent_size;
	uint64 transient_size;

	platform_api_functions platform_api;

	memory_arena debug_arena;
	memory_arena renderer_arena;
} game_memory;

#define GAME_INIT_FUNCTION(name) bool name (game_memory* memory)
typedef GAME_INIT_FUNCTION(game_init_function);

#define GAME_UPDATE_FUNCTION(name) void name (game_memory* memory, float delta_t)
typedef GAME_UPDATE_FUNCTION(game_update_function);

#define GAME_CLEANUP_FUNCTION(name) void name (game_memory* memory)
typedef GAME_CLEANUP_FUNCTION(game_cleanup_function);
