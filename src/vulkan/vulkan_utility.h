#pragma once

enum RENDERER_IMAGE_FORMAT
{
	ImageFormat_RGB8,
	ImageFormat_SRGBA8,
	ImageFormat_BC1_RGB,
	ImageFormat_BC1_SRGB,
	ImageFormat_BC2,
	ImageFormat_BC3,
};

enum RENDERER_IMAGE_USAGE
{
	ImageUsage_NormalTexture
};

struct image_handle
{
	u64 handle;
	u64 view_handle;
	u64 memory;
};

inline u32
VulkanSelectMemoryType(VkPhysicalDeviceMemoryProperties* memory_properties, u32 memory_type_bits, VkMemoryPropertyFlags property_flags)
{
    u32 selected_type_index = UINT32_MAX;
    
    for (u32 i = 0; i < memory_properties->memoryTypeCount; ++i)
    {
        if ((memory_type_bits & (1 << i)) != 0 && (memory_properties->memoryTypes[i].propertyFlags & property_flags) == property_flags)
        {
            selected_type_index = i;
            break;
        }
    }
    
    return selected_type_index;
}

inline image_handle
VulkanCreateImage(vulkan_renderer_state* state, u32 width, u32 height,
				  enum32(RENDERER_IMAGE_FORMAT) format, enum32(RENDERER_IMAGE_USAGE) usage,
				  u32 mip_levels = 1)
{
	image_handle result = {};
    
	VkFormat vulkan_image_format;
	VkImageUsageFlagBits vulkan_image_usage;
    
	switch(format)
	{
		case ImageFormat_RGB8:
        vulkan_image_format = VK_FORMAT_R8G8B8_UNORM;
		break;
        
		case ImageFormat_SRGBA8:
        vulkan_image_format = VK_FORMAT_R8G8B8A8_SRGB;
		break;
        
		case ImageFormat_BC1_RGB:
        vulkan_image_format = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		break;
        
		case ImageFormat_BC1_SRGB:
        vulkan_image_format = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
		break;
        
		case ImageFormat_BC2:
        vulkan_image_format = VK_FORMAT_BC2_SRGB_BLOCK;
		break;
        
		case ImageFormat_BC3:
        vulkan_image_format = VK_FORMAT_BC3_SRGB_BLOCK;
		break;
        
		INVALID_DEFAULT_CASE;
	}
    
	switch(usage)
	{
		case ImageUsage_NormalTexture:
        vulkan_image_usage = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		break;
        
		INVALID_DEFAULT_CASE;
	}
    
	VkImageCreateInfo create_info = {};
	create_info.sType		 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.format		= vulkan_image_format;
	create_info.extent		= {width, height, 1};
	create_info.mipLevels	 = mip_levels;
	create_info.arrayLayers   = 1;
	create_info.samples	   = VK_SAMPLE_COUNT_1_BIT;
	create_info.tiling		= VK_IMAGE_TILING_OPTIMAL;
	create_info.usage		 = vulkan_image_usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
	VkImage image = VK_NULL_HANDLE;
    
	// TODO(soimn): error handling
	state->vkCreateImage(state->device, &create_info, NULL, &image);
    
	VkMemoryRequirements memory_requirements;
	state->vkGetImageMemoryRequirements(state->device, image, &memory_requirements);
    
	u32 memory_type_index = VulkanSelectMemoryType(&state->physical_device_memory_properties, memory_requirements.memoryTypeBits,
												   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VkDeviceMemory memory = VK_NULL_HANDLE;
    {
        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType		   = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize  = memory_requirements.size;
        allocate_info.memoryTypeIndex = memory_type_index;
        
        // TODO(soimn): error handling
        state->vkAllocateMemory(state->device, &allocate_info, NULL, &memory);
        state->vkBindImageMemory(state->device, image, memory, NULL);
    }
    
	VkImageView image_view = VK_NULL_HANDLE;
	
	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image      = image;
	view_create_info.viewType   = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format     = vulkan_image_format;
	view_create_info.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, 
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    
	view_create_info.subresourceRange.aspectMask	 = VK_IMAGE_ASPECT_COLOR_BIT;
	view_create_info.subresourceRange.baseMipLevel   = 0;
	view_create_info.subresourceRange.levelCount	 = mip_levels;
	view_create_info.subresourceRange.layerCount	 = 1;
    
	result.handle	   = (u64) image;
	result.view_handle  = (u64) image_view;
	result.memory	   = (u64) memory;
    
	return result;
}
