#pragma once

#include <vulkan/vulkan.h>

#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "utils/fixed_int.h"

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

/// Vulkan

#define MAX_VULKAN_APP_NAME 60
#define ANT_VULKAN_INSTANCE_EXTENSION_COUNT_LIMIT 16
#define ANT_VULKAN_INSTANCE_LAYER_COUNT_LIMIT     16
#define ANT_VULKAN_DEVICE_EXTENSION_COUNT_LIMIT   16

struct vulkan_queue_family_info {
	int32 gfx_family;
	int32 present_family;
	int32 compute_family;
	int32 transfer_family;
};

typedef struct vulkan_application
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

	// TODO(soimn): setup a proper system handling these
	VkPipelineLayout debug_pipeline_layout;
	VkRenderPass debug_render_pass;
	VkPipeline debug_pipeline;
	VkFramebuffer* debug_framebuffers;
	uint32 debug_framebuffer_count;
	VkCommandPool debug_cmd_pool;
	VkCommandBuffer debug_cmd_buffers[16];
	uint32 debug_cmd_buffer_count;
	VkSemaphore debug_image_avail;
	VkSemaphore debug_render_done;

	const char* layers[ANT_VULKAN_INSTANCE_LAYER_COUNT_LIMIT];
	const char* extensions[ANT_VULKAN_INSTANCE_EXTENSION_COUNT_LIMIT];

	uint32 layer_count;
	uint32 extension_count;

	const char* device_extensions[ANT_VULKAN_DEVICE_EXTENSION_COUNT_LIMIT];
	uint32 device_extension_count;

	VkDebugUtilsMessengerEXT debug_messenger;

	bool has_compute_queue;
	bool has_separate_present_queue;

	vulkan_queue_family_info queue_families;

	VkQueue gfx_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;
	VkQueue compute_queue;

	void* library_module;
	
	#define VK_DEVICE_LEVEL_FUNCTION(func) PFN_##func func
	#include "vulkan_app_functions.inl"
	#undef VK_DEVICE_LEVEL_FUNCTION
} vulkan_application;

typedef struct platform_api_functions
{
	platform_log_info* LogInfo;
	platform_log_error* LogError;

	debug_read_file* DebugReadFile;
	debug_write_file* DebugWriteFile;
	debug_free_file_memory* DebugFreeFileMemory;
} platform_api_functions;

typedef struct game_memory
{
	bool is_initialized;

	void* persistent_memory;
	void* transient_memory;
	uint64 persistent_size;
	uint64 transient_size;

	platform_api_functions platform_api;
	vulkan_application* vk_application;

	memory_arena debug_arena;
} game_memory;

#define GAME_INIT_FUNCTION(name) void name (game_memory* memory)
typedef GAME_INIT_FUNCTION(game_init_function);

#define GAME_UPDATE_FUNCTION(name) void name (game_memory* memory, float delta_t)
typedef GAME_UPDATE_FUNCTION(game_update_function);
