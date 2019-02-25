#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#undef VK_NO_PROTOTYPES

#ifdef ANT_DEBUG
#define ASSERTION_ENABLED
#endif

#include "utils/assert.h"
#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "ant_types.h"

#ifdef ANT_PLATFORM_WINDOWS
#define EXPORT extern "C" __declspec(dllexport)
#endif

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
	b32 is_valid;
};

struct platform_file_info
{
	u64 timestamp;
	u64 file_size;
	char* base_name;
	void* platform_data;

	platform_file_info* next;
};

struct platform_file_group
{
	u32 file_count;
	platform_file_info* first_file_info;
	void* platform_data;
};

enum PlatformFileTypeTag
{
	PlatformFileType_AssetFile,
	PlatformFileType_SaveFile,
	PlatformFileType_PNG,
	PlatformFileType_WAV,
	PlatformFileType_OBJ,

	// NOTE(soimn): this is mostly for debugging
	PlatformFileType_ShaderFile,

	PlatformFileType_TagCount
};

enum PlatformOpenFileOpenFlags
{
	OpenFile_Read  = 0x1,
	OpenFile_Write = 0x2
};

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(name) platform_file_group name (enum32(PlatformFileTypeTag) file_type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN_FUNCTION(platform_get_all_files_of_type_begin_function);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(name) void name (platform_file_group* file_group)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END_FUNCTION(platform_get_all_files_of_type_end_function);

#define PLATFORM_OPEN_FILE_FUNCTION(name) platform_file_handle name (platform_file_info* file_info, u8 open_params)
typedef PLATFORM_OPEN_FILE_FUNCTION(platform_open_file_function);

#define PLATFORM_CLOSE_FILE_FUNCTION(name) void name (platform_file_handle* file_handle)
typedef PLATFORM_CLOSE_FILE_FUNCTION(platform_close_file_function);

#define PLATFORM_READ_FROM_FILE_FUNCTION(name) void name (platform_file_handle* handle, u64 offset, u64 size, void* dest)
typedef PLATFORM_READ_FROM_FILE_FUNCTION(platform_read_from_file_function);

#define PLATFORM_WRITE_TO_FILE_FUNCTION(name) void name (platform_file_handle* handle, u64 offset, u64 size, void* source)
typedef PLATFORM_WRITE_TO_FILE_FUNCTION(platform_write_to_file_function);

#define PLATFORM_FILE_IS_VALID(handle) ((handle)->is_valid)

typedef struct vulkan_api_functions
{
	#define VK_EXPORTED_FUNCTION(func) PFN_##func func
	#define VK_GLOBAL_LEVEL_FUNCTION(func) PFN_##func func
	#define VK_INSTANCE_LEVEL_FUNCTION(func) PFN_##func func
	#define VK_DEVICE_LEVEL_FUNCTION(func) PFN_##func func

	#include "vulkan_functions.inl"

	#undef VK_DEVICE_LEVEL_FUNCTION
	#undef VK_INSTANCE_LEVEL_FUNCTION
	#undef VK_GLOBAL_LEVEL_FUNCTION
	#undef VK_EXPORTED_FUNCTION
} vulkan_api;

typedef struct vulkan_renderer_state
{
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkSurfaceKHR surface;

	struct swapchain
	{
		VkSwapchainKHR handle;
		u32 image_count;
		VkImage* images;

		VkImageSubresourceRange subresource_range;
		VkExtent2D extent;
		VkSurfaceFormatKHR surface_format;
		VkPresentModeKHR present_mode;

		VkSemaphore image_available_semaphore;
		VkSemaphore transfer_done_semaphore;

		VkCommandPool present_pool;
		VkCommandBuffer* present_buffers;

		bool should_recreate_swapchain;
	} swapchain;

	struct render_target
	{
		VkImage image;
		VkImageView image_view;
		VkDeviceMemory image_memory;

		VkExtent2D extent;

		VkRenderPass render_pass;
		VkFramebuffer framebuffer;

		VkSemaphore render_done_semaphore;

		VkCommandPool render_pool;
		VkCommandBuffer render_buffer;
	} render_target;

	bool supports_compute, supports_dedicated_transfer;
	i32 graphics_family, compute_family, dedicated_transfer_family, present_family;
	VkQueue graphics_queue, compute_queue, transfer_queue, dedicated_transfer_queue, present_queue;
} vulkan_renderer_state;

typedef struct platform_api_functions
{
	platform_log_info_function* LogInfo;
	platform_log_error_function* LogError;

	platform_get_all_files_of_type_begin_function* GetAllFilesOfTypeBegin;
	platform_get_all_files_of_type_end_function* GetAllFilesOfTypeEnd;
	platform_open_file_function* OpenFile;
	platform_close_file_function* CloseFile;
	platform_read_from_file_function* ReadFromFile;
	platform_write_to_file_function* WriteToFile;
} platform_api_functions;

typedef struct game_memory
{
	bool is_initialized;

	platform_api_functions platform_api;
	vulkan_api_functions vulkan_api;

	vulkan_renderer_state vulkan_state;

	default_memory_arena_allocation_routines default_allocation_routines;
	memory_arena persistent_arena;
	memory_arena transient_arena;
	memory_arena debug_arena;
	memory_arena renderer_arena;
} game_memory;

#define GAME_INIT_FUNCTION(name) bool name (game_memory* memory)
typedef GAME_INIT_FUNCTION(game_init_function);

#define GAME_UPDATE_AND_RENDER_FUNCTION(name) void name (game_memory* memory, float delta_t)
typedef GAME_UPDATE_AND_RENDER_FUNCTION(game_update_and_render_function);

#define GAME_CLEANUP_FUNCTION(name) void name (game_memory* memory)
typedef GAME_CLEANUP_FUNCTION(game_cleanup_function);
