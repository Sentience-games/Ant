#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#undef VK_NO_PROTOTYPES

#include "ant_types.h"

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

		v2 immediate_viewport_dimensions;
		VkPipelineLayout immediate_layout;
		VkPipeline immediate_pipeline;
	} render_target;

	bool supports_compute, supports_dedicated_transfer;
	i32 graphics_family, compute_family, dedicated_transfer_family, present_family;
	VkQueue graphics_queue, compute_queue, transfer_queue, dedicated_transfer_queue, present_queue;
} vulkan_renderer_state;
