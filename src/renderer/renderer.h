#pragma once

// TODO(soimn):
/*
 * - Refactor memory acquisition when freeing of memory is implemented
 * - Check every switch statement and ensure the default case is invalid
 * - Remove any remaining assertions and replace them with predicated statements
 *   concerning the return value of query calls
 */

#ifdef PLATFORM_WINDOWS
#ifdef RENDERER_INTERNAL
#define RENDERER_EXPORT extern "C" __declspec(dllexport)
#else
#define RENDERER_EXPORT extern "C" __declspec(dllimport)
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#undef VK_NO_PROTOTYPES

#ifdef PLATFORM_WINDOWS
#undef VK_USE_PLATFORM_WIN32_KHR
#endif

#define RENDERER_MINIMUM_REQUIRED_MEMORY MEGABYTES(64)
#define RENDERER_DEFAULT_SWAPCHAIN_IMAGE_COUNT 3
#define RENDERER_DEFAULT_SWAPCHAIN_WIDTH 1920
#define RENDERER_DEFAULT_SWAPCHAIN_HEIGHT 1080

#include "utils/utility_defines.h"
#include "ant_types.h"
#include "utils/cstring.h"
#include "utils/memory_utils.h"

#include "math/vector.h"

// TODO(soimn):
/*
 * - The FPS drops after the window has been shaken for a while, fix this
 * - Implement custom memory allocation scheme
 */

/// TYPES

typedef VkBuffer vertex_buffer;
typedef VkBuffer index_buffer;
typedef VkBuffer uniform_buffer;

typedef VkDeviceMemory vertex_buffer_memory;
typedef VkDeviceMemory index_buffer_memory;
typedef VkDeviceMemory uniform_buffer_memory;

struct vertex
{
	v3 position, normal;
	v2 uv;
};

/// DEFAULT DATA

global_variable VkVertexInputBindingDescription
	default_vertex_binding_descriptions[]   =  {0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX};

global_variable VkVertexInputAttributeDescription
	default_vertex_attribute_descriptions[] = {{0, 0, VK_FORMAT_R32G32B32_SFLOAT, OFFSETOF(vertex, position)},
											   {1, 0, VK_FORMAT_R32G32B32_SFLOAT, OFFSETOF(vertex, normal)},
											   {2, 0, VK_FORMAT_R32G32_SFLOAT,	  OFFSETOF(vertex, uv)}};

/// INFO STRUCTS

struct vulkan_api
{
	#define VK_EXPORTED_FUNCTION(func) PFN_##func func
	#define VK_GLOBAL_LEVEL_FUNCTION(func) PFN_##func func
	#define VK_INSTANCE_LEVEL_FUNCTION(func) PFN_##func func
	#define VK_DEVICE_LEVEL_FUNCTION(func) PFN_##func func
	
	#include "vulkan_functions.inl"
	
	#undef VK_EXPORTED_FUNCTION
	#undef VK_GLOBAL_LEVEL_FUNCTION
	#undef VK_INSTANCE_LEVEL_FUNCTION
	#undef VK_DEVICE_LEVEL_FUNCTION
};

struct renderer_state
{
	void* vulkan_module;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkSurfaceKHR surface;

	struct
	{
		VkSwapchainKHR handle;
		u32 image_count;
		VkImage* images;
		VkImageView* image_views;
		VkExtent2D extent;
		VkFormat format;
		VkFramebuffer* framebuffers;

		u32 current_image_index;
		u32 command_buffer_count;
		VkCommandBuffer* command_buffers;
		VkCommandPool command_pool;

		VkSemaphore image_acquired_semaphore;
		VkSemaphore render_done_semaphore;
	} swapchain;

	struct
	{
		VkRenderPass render_pass;
	} defaults;

	bool supports_dedicated_transfer, supports_compute;
	int8 graphics_family, compute_family, dedicated_transfer_family, present_family;
	VkQueue graphics_queue, compute_queue, transfer_queue, dedicated_transfer_queue, present_queue;

	VkCommandPool temporary_transfer_pool;
};

enum RENDERER_ERROR_MESSAGE_SEVERITY
{
	RENDERER_ERROR_MESSAGE	 = 0x4,
	RENDERER_WARNING_MESSAGE = 0x3,
	RENDERER_INFO_MESSAGE	 = 0x2,
	RENDERER_DEBUG_MESSAGE	 = 0x1
};

typedef void error_callback (const char* message, const char* function_name,
							 unsigned int line_nr, RENDERER_ERROR_MESSAGE_SEVERITY severity);

struct vulkan_additional_logical_device_create_info
{
	const float priorities[4];

	const void* extension_ptr;
	VkDeviceCreateFlags create_flags;
	uint32 extension_count;
	const char** extensions;
	const VkPhysicalDeviceFeatures* features;
};

struct vulkan_swapchain_create_info
{
	VkFormat format;
	VkExtent2D extent;
	VkColorSpaceKHR color_space;
	uint32 image_count;
	VkImageUsageFlags image_usage_flags;

	bool use_different_present_mode;
	VkPresentModeKHR present_mode;

	bool disable_clipping;

	bool enable_image_sharing;
	uint32 whitelisted_family_count;
	const uint32* whitelisted_families;

	bool enable_stereoscopic_3d;
	uint32 image_view_count;

	bool use_transform;
	VkSurfaceTransformFlagBitsKHR transform_flags;

	bool use_compositing;
	VkCompositeAlphaFlagBitsKHR compositing_flags;

	bool reuse_swapchain;
	VkSwapchainKHR old_swapchain;

	// Extensions
	const void* extension_ptr;
	VkSwapchainCreateFlagsKHR creation_flags;
};

struct vulkan_swapchain_image_create_info
{
	VkFormat format;

	bool use_different_subresource_range;
	VkImageSubresourceRange subresource_range;

	bool use_different_mapping;
	VkComponentMapping component_mapping;

	bool unordinary_view_type;
	VkImageViewType view_type;

	VkImageViewCreateFlags create_flags;
	const void* extension_ptr;
};

struct vulkan_render_pass_create_info
{
	bool use_default_attachment;
	uint32 attachment_count;
	VkAttachmentDescription* attachments;

	bool use_default_subpass;
	uint32 subpass_count;
	VkSubpassDescription* subpasses;

	uint32 subpass_dependency_count;
	VkSubpassDependency* subpass_dependencies;

	const void* extension_ptr;
	VkRenderPassCreateFlags create_flags;
};

struct vulkan_shader_stage_create_info
{
	VkShaderModule vertex_shader;
	const char* vertex_main_function_name;
	const VkSpecializationInfo* vertex_specialization_info;

	VkShaderModule fragment_shader;
	const char* fragment_main_function_name;
	const VkSpecializationInfo* fragment_specialization_info;

	VkShaderModule geometry_shader;
	const char* geometry_main_function_name;
	const VkSpecializationInfo* geometry_specialization_info;
};

struct vulkan_graphics_pipeline_create_info
{
	uint32 pipeline_count;

	bool use_default_fixed_functions;
	bool use_pointer_arrays;

	bool use_multiple_render_passes;
	VkRenderPass* render_pass;

	bool use_single_pipeline_layout;
	VkPipelineLayout* pipeline_layout;

	bool use_same_shader_stage_count;
	uint32* shader_stage_count;
	VkPipelineShaderStageCreateInfo* shader_stage_info;

	bool use_default_vertex_input_state;
    VkPipelineVertexInputStateCreateInfo* vertex_input_state;
	bool use_default_input_assembly_state;
    VkPipelineInputAssemblyStateCreateInfo* input_assembly_state;
    VkPipelineTessellationStateCreateInfo* tessellation_state;
	bool use_default_viewport_state;
    VkPipelineViewportStateCreateInfo* viewport_state;
	bool use_default_rasterization_state;
    VkPipelineRasterizationStateCreateInfo* rasterization_state;
	bool use_default_multisample_state;
    VkPipelineMultisampleStateCreateInfo* multisampling_state;
	bool use_default_depth_stencil_state;
    VkPipelineDepthStencilStateCreateInfo* depth_stencil_state;
	bool use_default_color_blend_state;
    VkPipelineColorBlendStateCreateInfo* color_blend_state;
    VkPipelineDynamicStateCreateInfo* dynamic_state;
	
	uint32* subpass_index;

	VkPipeline* base_pipeline_handle;
	bool use_base_pipeline_index;
	int32* base_pipeline_index;
	
	const void** extension_ptr;
	VkPipelineCreateFlags* create_flags;
};

struct vulkan_framebuffer_create_info
{
	uint32 attachment_count;
	const VkImageView* attachments;
	VkExtent2D extent;
	bool use_width_and_height;
	uint32 width;
	uint32 height;
	uint32 layers;

	const void* extension_ptr;
	VkFramebufferCreateFlags create_flags;
};

struct vulkan_vertex_buffer_create_info
{
	void* vertex_data;
	VkDeviceSize size;
	VkSharingMode sharing_mode;

	const void* extension_ptr;
	VkBufferCreateFlags create_flags;
};

struct vulkan_index_buffer_create_info
{
	void* index_data;
	VkDeviceSize size;
	VkSharingMode sharing_mode;

	const void* extension_ptr;
	VkBufferCreateFlags create_flags;
};

struct vulkan_descriptor_pool_create_info
{
	uint32 max_descriptor_set_count;

	uint32 pool_size_count;
	const VkDescriptorPoolSize* pool_sizes;

	const void* extension_ptr;
	VkDescriptorPoolCreateFlags create_flags;
};

/// EXPORTED FUNCTIONS
#define RENDERER_VERIFY_PHYSICAL_DEVICE_EXTENSION_SUPPORT_FUNCTION(name) bool name (VkPhysicalDevice device, const char** req_extension_names,\
																					uint32 req_extension_count)
typedef RENDERER_VERIFY_PHYSICAL_DEVICE_EXTENSION_SUPPORT_FUNCTION(renderer_verify_physical_device_extension_support_function);
RENDERER_EXPORT RENDERER_VERIFY_PHYSICAL_DEVICE_EXTENSION_SUPPORT_FUNCTION(VulkanVerifyPhysicalDeviceExtensionSupport);

#define RENDERER_ACQUIRE_PHYSICAL_DEVICE_FUNCTION(name) bool name (const char** req_extension_names, uint32 req_extension_count)
typedef RENDERER_ACQUIRE_PHYSICAL_DEVICE_FUNCTION(renderer_acquire_physical_device_function);
RENDERER_EXPORT RENDERER_ACQUIRE_PHYSICAL_DEVICE_FUNCTION(VulkanAcquirePhysicalDevice);

RENDERER_EXPORT
#define RENDERER_CREATE_LOGICAL_DEVICE_FUNCTION(name) bool name (vulkan_additional_logical_device_create_info* additional_create_info)
typedef RENDERER_CREATE_LOGICAL_DEVICE_FUNCTION(renderer_create_logical_device_function);
RENDERER_EXPORT RENDERER_CREATE_LOGICAL_DEVICE_FUNCTION(VulkanCreateLogicalDevice);

#define RENDERER_VERIFY_LAYER_SUPPORT_FUNCTION(name) bool name (const char** req_layer_names, uint32 req_layer_count)
typedef RENDERER_VERIFY_LAYER_SUPPORT_FUNCTION(renderer_verify_layer_support_function);
RENDERER_EXPORT RENDERER_VERIFY_LAYER_SUPPORT_FUNCTION(VulkanVerifyLayerSupport);

#define RENDERER_VERIFY_INSTANCE_EXTENSION_SUPPORT_FUNCTION(name) bool name (const char** req_extension_names, uint32 req_extension_count)
typedef RENDERER_VERIFY_INSTANCE_EXTENSION_SUPPORT_FUNCTION(renderer_verify_instance_extension_support_function);
RENDERER_EXPORT RENDERER_VERIFY_INSTANCE_EXTENSION_SUPPORT_FUNCTION(VulkanVerifyInstanceExtensionSupport);

#define RENDERER_CREATE_INSTANCE_FUNCTION(name) bool name (const char* engine_name, uint32 engine_version,\
														   const char* app_name, uint32 app_version,\
														   const char** layers, uint32 layer_count,\
														   const char** extensions, uint32 extension_count,\
														   const void* extension_ptr, VkInstanceCreateFlags create_flags)
typedef RENDERER_CREATE_INSTANCE_FUNCTION(renderer_create_instance_function);
RENDERER_EXPORT RENDERER_CREATE_INSTANCE_FUNCTION(VulkanCreateInstance);

#ifdef PLATFORM_WINDOWS
#define RENDERER_CREATE_SURFACE_FUNCTION(name) bool name (HINSTANCE process_instance, HWND window_handle,\
														  const void* extension_ptr, VkWin32SurfaceCreateFlagsKHR create_flags)
typedef RENDERER_CREATE_SURFACE_FUNCTION(renderer_create_surface_function);
RENDERER_EXPORT RENDERER_CREATE_SURFACE_FUNCTION(VulkanCreateWin32Surface);

#define RENDERER_INIT_FUNCTION(name) bool name (memory_arena* arena, error_callback* callback,\
												renderer_state*& state, vulkan_api*& api)
typedef RENDERER_INIT_FUNCTION(renderer_init_function);
RENDERER_EXPORT RENDERER_INIT_FUNCTION(Win32InitRenderer);
#endif

#define RENDERER_START_FUNCTION(name) bool name (void)
typedef RENDERER_START_FUNCTION(renderer_start_function);
RENDERER_EXPORT RENDERER_START_FUNCTION(RendererStart);

#define RENDERER_BEGIN_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_BEGIN_FRAME_FUNCTION(renderer_begin_frame_function);
RENDERER_EXPORT RENDERER_BEGIN_FRAME_FUNCTION(VulkanBeginFrame);

#define RENDERER_END_FRAME_FUNCTION(name) void name (void)
typedef RENDERER_END_FRAME_FUNCTION(renderer_end_frame_function);
RENDERER_EXPORT RENDERER_END_FRAME_FUNCTION(VulkanEndFrameFunction);

#define RENDERER_GET_OPTIMAL_SWAPCHAIN_EXTENT_FUNCTION(name) bool name (VkExtent2D* extent)
typedef RENDERER_GET_OPTIMAL_SWAPCHAIN_EXTENT_FUNCTION(renderer_get_optimal_swapchain_extent_function);
RENDERER_EXPORT RENDERER_GET_OPTIMAL_SWAPCHAIN_EXTENT_FUNCTION(VulkanGetOptimalSwapchainExtent);

#define RENDERER_GET_OPTIMAL_SWAPCHAIN_SURFACE_FORMAT_FUNCTION(name) bool name (VkSurfaceFormatKHR* format)
typedef RENDERER_GET_OPTIMAL_SWAPCHAIN_SURFACE_FORMAT_FUNCTION(renderer_get_optimal_swapchain_surface_format_function);
RENDERER_EXPORT RENDERER_GET_OPTIMAL_SWAPCHAIN_SURFACE_FORMAT_FUNCTION(VulkanGetOptimalSwapchainSurfaceFormat);

#define RENDERER_GET_OPTIMAL_SWAPCHAIN_PRESENT_MODE_FUNCTION(name) bool name (VkPresentModeKHR* present_mode)
typedef RENDERER_GET_OPTIMAL_SWAPCHAIN_PRESENT_MODE_FUNCTION(renderer_get_optimal_swapchain_present_mode_function);
RENDERER_EXPORT RENDERER_GET_OPTIMAL_SWAPCHAIN_PRESENT_MODE_FUNCTION(VulkanGetOptimalSwapchainPresentMode);

#define RENDERER_CREATE_SWAPCHAIN_FUNCTION(name) bool name (vulkan_swapchain_create_info* swapchain_create_info)
typedef RENDERER_CREATE_SWAPCHAIN_FUNCTION(renderer_create_swapchain_function);
RENDERER_EXPORT RENDERER_CREATE_SWAPCHAIN_FUNCTION(VulkanCreateSwapchain);

#define RENDERER_CREATE_SWAPCHAIN_IMAGES_FUNCTION(name) bool name (vulkan_swapchain_image_create_info* image_create_info)
typedef RENDERER_CREATE_SWAPCHAIN_IMAGES_FUNCTION(renderer_create_swapchain_images_function);
RENDERER_EXPORT RENDERER_CREATE_SWAPCHAIN_IMAGES_FUNCTION(VulkanCreateSwapchainImages);

#define RENDERER_CREATE_RENDER_PASS_FUNCTION(name) VkRenderPass name (vulkan_render_pass_create_info render_pass_create_info)
typedef RENDERER_CREATE_RENDER_PASS_FUNCTION(renderer_create_render_pass_function);
RENDERER_EXPORT RENDERER_CREATE_RENDER_PASS_FUNCTION(VulkanCreateRenderPass);

#define RENDERER_CREATE_DEFAULT_RENDER_PASS_FUNCTION(name) bool name (void)
typedef RENDERER_CREATE_DEFAULT_RENDER_PASS_FUNCTION(renderer_create_default_render_pass_function);
RENDERER_EXPORT RENDERER_CREATE_DEFAULT_RENDER_PASS_FUNCTION(VulkanCreateDefaultRenderPass);

#define RENDERER_CREATE_PIPELINE_LAYOUT_FUNCTION(name) VkPipelineLayout name (VkDescriptorSetLayout* descriptors, uint32 descriptor_count,\
																			  VkPushConstantRange* push_constant_ranges, uint32 push_constant_range_count)
typedef RENDERER_CREATE_PIPELINE_LAYOUT_FUNCTION(renderer_create_pipeline_layout_function);
RENDERER_EXPORT RENDERER_CREATE_PIPELINE_LAYOUT_FUNCTION(VulkanCreatePipelineLayout);

#define RENDERER_CREATE_SHADER_MODULE_FUNCTION(name) VkShaderModule name (uint32 shader_file_size, uint32* shader_file,\
																		  const void* extension_ptr, VkShaderModuleCreateFlags create_flags)
typedef RENDERER_CREATE_SHADER_MODULE_FUNCTION(renderer_create_shader_module_function);
RENDERER_EXPORT RENDERER_CREATE_SHADER_MODULE_FUNCTION(VulkanCreateShaderModule);

#define RENDERER_CREATE_SHADER_STAGE_INFOS_FUNCTION(name) bool name (vulkan_shader_stage_create_info stage_create_info, VkPipelineShaderStageCreateInfo* shader_stage_info_array,\
																	 uint32 shader_stage_info_array_size)
typedef RENDERER_CREATE_SHADER_STAGE_INFOS_FUNCTION(renderer_create_shader_stage_infos_function);
RENDERER_EXPORT RENDERER_CREATE_SHADER_STAGE_INFOS_FUNCTION(VulkanCreateShaderStageInfos);

#define RENDERER_CREATE_GRAPHICS_PIPELINES_FUNCTION(name) bool name (vulkan_graphics_pipeline_create_info gfx_pipeline_info, VkPipeline* pipeline_array,\
																	 uint32 pipeline_array_count)
typedef RENDERER_CREATE_GRAPHICS_PIPELINES_FUNCTION(renderer_create_graphics_pipelines_function);
RENDERER_EXPORT RENDERER_CREATE_GRAPHICS_PIPELINES_FUNCTION(VulkanCreateGraphicsPipelines);

#define RENDERER_CREATE_FRAMEBUFFER_FUNCTION(name) VkFramebuffer name (vulkan_framebuffer_create_info buffer_create_info, VkRenderPass render_pass)
typedef RENDERER_CREATE_FRAMEBUFFER_FUNCTION(renderer_create_framebuffer_function);
RENDERER_EXPORT RENDERER_CREATE_FRAMEBUFFER_FUNCTION(VulkanCreateFramebuffer);

#define RENDERER_CREATE_SWAPCHAIN_FRAMEBUFFERS_FUNCTION(name) bool name (void)
typedef RENDERER_CREATE_SWAPCHAIN_FRAMEBUFFERS_FUNCTION(renderer_create_swapchain_framebuffers_function);
RENDERER_EXPORT RENDERER_CREATE_SWAPCHAIN_FRAMEBUFFERS_FUNCTION(VulkanCreateSwapchainFramebuffers);

#define RENDERER_CREATE_COMMAND_POOL_FUNCTION(name) VkCommandPool name (uint32 queue_family, const void* extension_ptr, VkCommandPoolCreateFlags create_flags)
typedef RENDERER_CREATE_COMMAND_POOL_FUNCTION(renderer_create_command_pool_function);
RENDERER_EXPORT RENDERER_CREATE_COMMAND_POOL_FUNCTION(VulkanCreateCommandPool);

#define RENDERER_ALLOCATE_COMMAND_BUFFERS_FUNCTION(name) bool name (VkCommandPool command_pool, VkCommandBufferLevel buffer_level,\
																	uint32 buffer_count, VkCommandBuffer* buffers,\
																	const void* extension_ptr)
typedef RENDERER_ALLOCATE_COMMAND_BUFFERS_FUNCTION(renderer_allocate_command_buffers_function);
RENDERER_EXPORT RENDERER_ALLOCATE_COMMAND_BUFFERS_FUNCTION(VulkanAllocateCommandBuffers);

#define RENDERER_CREATE_SWAPCHAIN_COMMAND_BUFFERS_FUNCTION(name) bool name (void)
typedef RENDERER_CREATE_SWAPCHAIN_COMMAND_BUFFERS_FUNCTION(renderer_create_swapchain_command_buffers_function);
RENDERER_EXPORT RENDERER_CREATE_SWAPCHAIN_COMMAND_BUFFERS_FUNCTION(VulkanCreateSwapchainCommandBuffers);

#define RENDERER_CREATE_SEMAPHORE_FUNCTION(name) VkSemaphore name (const void* extension_ptr, VkSemaphoreCreateFlags create_flags)
typedef RENDERER_CREATE_SEMAPHORE_FUNCTION(renderer_create_semaphore_function);
RENDERER_EXPORT RENDERER_CREATE_SEMAPHORE_FUNCTION(VulkanCreateSemaphore);

#define RENDERER_CREATE_SWAPCHAIN_SEMAPHORES_FUNCTION(name) bool name (void)
typedef RENDERER_CREATE_SWAPCHAIN_SEMAPHORES_FUNCTION(renderer_create_swapchain_semaphores_function);
RENDERER_EXPORT RENDERER_CREATE_SWAPCHAIN_SEMAPHORES_FUNCTION(VulkanCreateSwapchainSemaphores);

#define RENDERER_COPY_BUFFER_FUNCTION(name) bool name (VkBuffer source, VkBuffer dest, VkDeviceSize size)
typedef RENDERER_COPY_BUFFER_FUNCTION(renderer_copy_buffer_function);
RENDERER_EXPORT RENDERER_COPY_BUFFER_FUNCTION(VulkanCopyBuffer);

#define RENDERER_CREATE_VERTEX_BUFFER_FUNCTION(name) bool name (vulkan_vertex_buffer_create_info buffer_create_info, VkDeviceMemory* buffer_memory,\
																VkBuffer* buffer)
typedef RENDERER_CREATE_VERTEX_BUFFER_FUNCTION(renderer_create_vertex_buffer_function);
RENDERER_EXPORT RENDERER_CREATE_VERTEX_BUFFER_FUNCTION(VulkanCreateVertexBuffer);

#define RENDERER_CREATE_INDEX_BUFFER_FUNCTION(name) bool name (vulkan_index_buffer_create_info buffer_create_info, VkDeviceMemory* buffer_memory,\
															   VkBuffer* buffer)
typedef RENDERER_CREATE_INDEX_BUFFER_FUNCTION(renderer_create_index_buffer_function);
RENDERER_EXPORT RENDERER_CREATE_INDEX_BUFFER_FUNCTION(VulkanCreateIndexBuffer);

#define RENDERER_CREATE_UNIFORM_BUFFER_FUNCTION(name) bool name (VkDeviceMemory* uniform_buffer_memory, VkDeviceSize uniform_buffer_size,\
																 VkBuffer* uniform_buffer)
typedef RENDERER_CREATE_UNIFORM_BUFFER_FUNCTION(renderer_create_uniform_buffer_function);
RENDERER_EXPORT RENDERER_CREATE_UNIFORM_BUFFER_FUNCTION(VulkanCreateUniformBuffer);

#define RENDERER_LOAD_MESH_FUNCTION(name) bool name (VkDeviceMemory* vertex_buffer_memory, VkBuffer* vertex_buffer,\
													 VkDeviceMemory* index_buffer_memory, VkBuffer* index_buffer,\
													 void* vertex_data, uint32 vertex_count,\
													 void* index_data, uint32 index_count)
typedef RENDERER_LOAD_MESH_FUNCTION(renderer_load_mesh_function);
RENDERER_EXPORT RENDERER_LOAD_MESH_FUNCTION(VulkanLoadMesh);

#define RENDERER_UPDATE_MESH_FUNCTION(name) bool name (VkBuffer* vertex_buffer, VkBuffer* index_buffer,\
													   void* vertex_data, uint32 vertex_count,\
													   void* index_data, uint32 index_count)
typedef RENDERER_UPDATE_MESH_FUNCTION(renderer_update_mesh_function);
RENDERER_EXPORT RENDERER_UPDATE_MESH_FUNCTION(VulkanUpdateMesh);

#define RENDERER_UNLOAD_MESH_FUNCTION(name) void name (VkDeviceMemory* vertex_buffer_memory, VkBuffer* vertex_buffer,\
													   VkDeviceMemory* index_buffer_memory, VkBuffer* index_buffer)
typedef RENDERER_UNLOAD_MESH_FUNCTION(renderer_unload_mesh_function);
RENDERER_EXPORT RENDERER_UNLOAD_MESH_FUNCTION(VulkanUnloadMesh);

#define RENDERER_CREATE_DESCRIPTOR_SET_LAYOUT_FUNCTION(name) VkDescriptorSetLayout name (VkDescriptorSetLayoutBinding* layout_bindings, uint32 layout_binding_count)
typedef RENDERER_CREATE_DESCRIPTOR_SET_LAYOUT_FUNCTION(renderer_create_descriptor_set_layout_function);
RENDERER_EXPORT RENDERER_CREATE_DESCRIPTOR_SET_LAYOUT_FUNCTION(VulkanCreateDescriptorSetLayout);

#define RENDERER_CREATE_DESCRIPTOR_POOL_FUNCTION(name) VkDescriptorPool name (vulkan_descriptor_pool_create_info pool_create_info)
typedef RENDERER_CREATE_DESCRIPTOR_POOL_FUNCTION(renderer_create_descriptor_pool_function);
RENDERER_EXPORT RENDERER_CREATE_DESCRIPTOR_POOL_FUNCTION(VulkanCreateDescriptorPool);

#define RENDERER_ALLOCATE_DESCRIPTOR_SETS_FUNCTION(name) bool name (VkDescriptorPool descriptor_pool, uint32 set_count,\
																	const VkDescriptorSetLayout* set_layouts, VkDescriptorSet* descriptor_sets,\
																	const void* extension_ptr)
typedef RENDERER_ALLOCATE_DESCRIPTOR_SETS_FUNCTION(renderer_allocate_descriptor_sets_function);
RENDERER_EXPORT RENDERER_ALLOCATE_DESCRIPTOR_SETS_FUNCTION(VulkanAllocateDescriptorSets);

#define RENDERER_UPDATE_UNIFORM_BUFFER_DESCRIPTOR_SET_FUNCTION(name) void name (VkBuffer uniform_buffer, VkDeviceSize buffer_offset,\
																				VkDeviceSize buffer_range, VkDescriptorSet descriptor_set,\
																				uint32 descriptor_binding, uint32 descriptor_array_element)
typedef RENDERER_UPDATE_UNIFORM_BUFFER_DESCRIPTOR_SET_FUNCTION(renderer_update_uniform_descriptor_set_function);
RENDERER_EXPORT RENDERER_UPDATE_UNIFORM_BUFFER_DESCRIPTOR_SET_FUNCTION(VulkanUpdateUniformBufferDescriptorSet);
