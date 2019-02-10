#pragma once

#include "ant_platform.h"

#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "utils/fixed_int.h"
#include "utils/cstring.h"

// TODO(soimn): implement proper assertions for use in game code
#include "utils/assert.h"

global_variable platform_api_functions* Platform;
global_variable platform_renderer_api* Renderer;
global_variable renderer_state* RendererState;
global_variable vulkan_api* VulkanAPI;

global_variable memory_arena* DebugArena;

struct vulkan_app {
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
};

global_variable vulkan_app* VulkanApp;

// TODO(soimn):
/*
 *	- Architecture a flexible and robust entity system with an
 *	  easy to use api for instantiation and parameter tweaking
 *	- Move the responsibility of creating a logical device to the game,
 *	  to allow platform independent creation of multiple devices and game
 *	  specific storage of handles, such as the device handle and the swapchain
 *	  handle
 */
