#include "ant.h"

/// Vulkan

internal VkRenderPass
VulkanCreateRenderPass()
{
	VkRenderPass render_pass;

    VkAttachmentDescription color_attachment = {};
	color_attachment.flags = 0;
    color_attachment.format = VulkanApp->swapchain_image_format;

	// NOTE(soimn): this is for multisampling
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	
	// NOTE(soimn): this is for changing the behaviour of attachment loading,
	//				in case someone wants to execute predicated behaviour using the previous data
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	// NOTE(soimn): this is for changing the behaviour of store operations,
	//				in case someone wants to discard after the render pass
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// NOTE(soimn): this is identical to the previous fields,
	//				except they apply to stencil operations instead of color operations
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// NOTE(soimn): this specifies the role of the image before and after the render pass,
	//				possible values are color, present and transfer
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_reference = {};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};

	// NOTE(soimn): this specifies that the subpass is graphics related
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;

	// NOTE(soimn): this is the attachment which is to be referenced in the output of the fragment shader,
	//				e.g. "layout(location = 0) out vec4 color".
	//				Possible attachments: color, input, resolve, depth and preserved.
	subpass.pColorAttachments = &color_attachment_reference;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.pNext = NULL;
	render_pass_create_info.flags = 0;
	render_pass_create_info.attachmentCount = 1;
	render_pass_create_info.pAttachments = &color_attachment;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &dependency;

	VkResult result = VulkanApp->vkCreateRenderPass(VulkanApp->device, &render_pass_create_info, NULL, &render_pass);
	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;
			
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return render_pass;
}

internal VkShaderModule
VulkanCreateShaderModule(platform_file_path shader_code)
{
	VkShaderModule module = VK_NULL_HANDLE;

	if (shader_code != NULL)
	{
		// TODO(soimn): setup proper file reading
		platform_file_handle shader_file = Platform->DebugReadFile(shader_code, alignof(uint32));

		VkShaderModuleCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.pNext = NULL;
		create_info.flags = 0;
		create_info.codeSize = shader_file.size;
		create_info.pCode = (uint32*) shader_file.data;

		VkResult result = VulkanApp->vkCreateShaderModule(VulkanApp->device, &create_info, NULL, &module);
		if (result != VK_SUCCESS)
		{
			switch(result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
				break;
				
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;
				
				case VK_ERROR_INVALID_SHADER_NV:
					LOG_ERROR("VK_ERROR_INVALID_SHADER_NV");
				break;

				default:
					Assert(false);
				break;
			}
		}
	}

	return module;
}

internal VkPipelineLayout
VulkanCreatePipelineLayout(VkDescriptorSetLayout** descriptors, uint32 descriptor_count,
						  VkPushConstantRange** push_constant_ranges, uint32 push_constant_range_count)
{
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	VkResult result;

	VkPipelineLayoutCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_create_info.setLayoutCount = descriptor_count;
	pipeline_create_info.pSetLayouts = (descriptors ? descriptors[0] : NULL);
	pipeline_create_info.pushConstantRangeCount = push_constant_range_count;
	pipeline_create_info.pPushConstantRanges = (push_constant_ranges ? push_constant_ranges[0] : NULL);

	result = VulkanApp->vkCreatePipelineLayout(VulkanApp->device, &pipeline_create_info, NULL, &pipeline_layout);
	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;
			
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return pipeline_layout;
}

struct graphics_pipeline_create_info
{
	platform_file_path vertex_shader;
	platform_file_path fragment_shader;
	platform_file_path geometry_shader;
	platform_file_path tesselation_shader;

	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
};

// TODO(soimn): refactor
internal VkPipeline
VulkanCreateGraphicsPipeline(graphics_pipeline_create_info* gfx_pipeline_info)
{
	VkResult result;
	VkPipeline pipeline = VK_NULL_HANDLE;

	bool has_geometry_shader = gfx_pipeline_info->geometry_shader != NULL;
	bool has_tesselation_shader = gfx_pipeline_info->tesselation_shader != NULL;

	VkShaderModule vertex_shader_module = VulkanCreateShaderModule(gfx_pipeline_info->vertex_shader);
	VkShaderModule fragment_shader_module = VulkanCreateShaderModule(gfx_pipeline_info->fragment_shader);
	VkShaderModule geometry_shader_module = VulkanCreateShaderModule(gfx_pipeline_info->geometry_shader);
	VkShaderModule tesselation_shader_module = VulkanCreateShaderModule(gfx_pipeline_info->tesselation_shader);

	uint32 shader_stage_count = 0;
	VkPipelineShaderStageCreateInfo shader_stage_create_info[4] = {};

	shader_stage_create_info[shader_stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_create_info[shader_stage_count].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_create_info[shader_stage_count].module = vertex_shader_module;
	shader_stage_create_info[shader_stage_count].pName = "main";
	shader_stage_create_info[shader_stage_count].pSpecializationInfo = NULL;
	++shader_stage_count;

	shader_stage_create_info[shader_stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_create_info[shader_stage_count].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_create_info[shader_stage_count].module = fragment_shader_module;
	shader_stage_create_info[shader_stage_count].pName = "main";
	shader_stage_create_info[shader_stage_count].pSpecializationInfo = NULL;
	++shader_stage_count;

	if (has_geometry_shader)
	{
		shader_stage_create_info[shader_stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_create_info[shader_stage_count].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		shader_stage_create_info[shader_stage_count].module = geometry_shader_module;
		shader_stage_create_info[shader_stage_count].pName = "main";
		shader_stage_create_info[shader_stage_count].pSpecializationInfo = NULL;
		++shader_stage_count;
	}

// TODO(soimn): figure out which enumeration corresponds to the tesselation shader
// 	if (has_tesselation_shader)
// 	{
// 		shader_stage_create_info[shader_stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
// 		shader_stage_create_info[shader_stage_count].stage = VK_SHADER_STAGE_TESSELATION_BIT;
// 		shader_stage_create_info[shader_stage_count].module = tesselation_shader_module;
// 		shader_stage_create_info[shader_stage_count].pName = "main";
// 		shader_stage_create_info[shader_stage_count].pSpecializationInfo = NULL;
// 		++shader_stage_count;
// 	}

	// TODO(soimn): change this to support instancing and attributes
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	// TODO(soimn): change this to support different topology
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// TODO(soimn): setup support for user-defined viewport options
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) VulkanApp->swapchain_extent.width;
	viewport.height = (float) VulkanApp->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// TODO(soimn): change this to support user-defines scissoring
	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = VulkanApp->swapchain_extent;

	// TODO(soimn): consider supporting multiple scissors and viewports
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// TODO(soimn): setup support for predicated wireframe rendering (this requires a gpu feature)
	// TODO(soimn): consider supporting different lineWidths (this requires a gpu feature)
	// TODO(soimn): support changing culling mode
	// TODO(soimn): support custom depth bias for shadow mapping and stuff...
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	// TODO(soimn): enable multisampling (this requires a gpu feature)
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// TODO(soimn): setup depth and stencil testing

	// TODO(soimn): consider supporting global color blending with VkPipelineColorBlendStateCreateInfo
	// TODO(soimn): setup proper color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

// TODO(soimn): implement support for dynamic state
// 	VkDynamicState dynamicStates[] = {
// 		VK_DYNAMIC_STATE_VIEWPORT,
// 		VK_DYNAMIC_STATE_LINE_WIDTH
// 	};
// 
// 	VkPipelineDynamicStateCreateInfo dynamicState = {};
// 	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
// 	dynamicState.dynamicStateCount = 2;
// 	dynamicState.pDynamicStates = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = NULL;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = shader_stage_count;
	pipelineInfo.pStages = &shader_stage_create_info[0];
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = gfx_pipeline_info->pipeline_layout;
	pipelineInfo.renderPass = gfx_pipeline_info->render_pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	result = VulkanApp->vkCreateGraphicsPipelines(VulkanApp->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;
			
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;
			
			case VK_ERROR_INVALID_SHADER_NV:
				LOG_ERROR("VK_ERROR_INVALID_SHADER_NV");
			break;

			default:
				Assert(false);
			break;
		}
	}

	VulkanApp->vkDestroyShaderModule(VulkanApp->device, vertex_shader_module, NULL);
	VulkanApp->vkDestroyShaderModule(VulkanApp->device, fragment_shader_module, NULL);
	if (has_geometry_shader) VulkanApp->vkDestroyShaderModule(VulkanApp->device, geometry_shader_module, NULL);
	if (has_tesselation_shader) VulkanApp->vkDestroyShaderModule(VulkanApp->device, tesselation_shader_module, NULL);

	return pipeline;
}

// TODO(soimn): cleanup framebuffers
internal bool
VulkanCreateDebugFramebuffers(VkRenderPass render_pass)
{
	bool succeeded = false;

	VulkanApp->debug_framebuffer_count = VulkanApp->swapchain_image_view_count;
	VulkanApp->debug_framebuffers = PushArray(DebugArena, VkFramebuffer, VulkanApp->debug_framebuffer_count);

	uint32 i = 0;
	for (; i < VulkanApp->swapchain_image_view_count; i++) {
		VkImageView attachments[] = {VulkanApp->swapchain_image_views[i]};

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = render_pass;
		framebuffer_create_info.attachmentCount = 1;
		framebuffer_create_info.pAttachments = attachments;
		framebuffer_create_info.width = VulkanApp->swapchain_extent.width;
		framebuffer_create_info.height = VulkanApp->swapchain_extent.height;
		framebuffer_create_info.layers = 1;

		VkResult result = VulkanApp->vkCreateFramebuffer(VulkanApp->device, &framebuffer_create_info, NULL, &VulkanApp->debug_framebuffers[i]);
		if (result != VK_SUCCESS)
		{
			switch (result)
			{
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
				break;
				
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;
				
				case VK_ERROR_INVALID_SHADER_NV:
					LOG_ERROR("VK_ERROR_INVALID_SHADER_NV");
				break;

				default:
					Assert(false);
				break;
			}

			break;
		}
	}

	if (i == VulkanApp->swapchain_image_view_count + 1)
	{
		succeeded = true;
	}

	return succeeded;
}

// // TODO(soimn): cleanup
internal VkCommandPool
VulkanCreateCommandPool(uint32 queue_family)
{
	VkCommandPool command_pool = VK_NULL_HANDLE;

	VkCommandPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.flags = 0;
	pool_create_info.queueFamilyIndex = queue_family;

	VkResult result = VulkanApp->vkCreateCommandPool(VulkanApp->device, &pool_create_info, nullptr, &command_pool);
	if (result != VK_SUCCESS)
	{
		switch (result)
		{
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
			break;
			
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				LOG_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
			break;
			
			case VK_ERROR_INVALID_SHADER_NV:
				LOG_ERROR("VK_ERROR_INVALID_SHADER_NV");
			break;

			default:
				Assert(false);
			break;
		}
	}

	return command_pool;
}

extern "C" GAME_INIT_FUNCTION(GameInit)
{
	Platform = &memory->platform_api;
	VulkanApp = memory->vk_application;

	DebugArena = &memory->debug_arena;

	if (!memory->is_initialized)
	{
		VulkanApp->debug_render_pass = VulkanCreateRenderPass();
		VulkanApp->debug_pipeline_layout = VulkanCreatePipelineLayout(NULL, 0, NULL, 0);
		graphics_pipeline_create_info pip_create_info = {"a:\\res\\vert.spv", "a:\\res\\frag.spv", NULL, NULL,
														VulkanApp->debug_render_pass, VulkanApp->debug_pipeline_layout};
		VulkanApp->debug_pipeline = VulkanCreateGraphicsPipeline(&pip_create_info);
		VulkanCreateDebugFramebuffers(VulkanApp->debug_render_pass);
		VulkanApp->debug_cmd_buffer_count = VulkanApp->debug_framebuffer_count;
		VulkanApp->debug_cmd_pool = VulkanCreateCommandPool(VulkanApp->queue_families.gfx_family);
		VkCommandBufferAllocateInfo buffer_alloc_info = {};
		buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buffer_alloc_info.commandPool = VulkanApp->debug_cmd_pool;
		buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		buffer_alloc_info.commandBufferCount = VulkanApp->debug_cmd_buffer_count;

		VulkanApp->vkAllocateCommandBuffers(VulkanApp->device, &buffer_alloc_info, VulkanApp->debug_cmd_buffers);

		for (size_t i = 0; i < VulkanApp->debug_cmd_buffer_count; i++) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			VulkanApp->vkBeginCommandBuffer(VulkanApp->debug_cmd_buffers[i], &beginInfo);

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = VulkanApp->debug_render_pass;
			renderPassInfo.framebuffer = VulkanApp->debug_framebuffers[i];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent = VulkanApp->swapchain_extent;

			VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			VulkanApp->vkCmdBeginRenderPass(VulkanApp->debug_cmd_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			VulkanApp->vkCmdBindPipeline(VulkanApp->debug_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanApp->debug_pipeline);
			VulkanApp->vkCmdDraw(VulkanApp->debug_cmd_buffers[i], 3, 1, 0, 0);
			VulkanApp->vkCmdEndRenderPass(VulkanApp->debug_cmd_buffers[i]);
			VulkanApp->vkEndCommandBuffer(VulkanApp->debug_cmd_buffers[i]);

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VulkanApp->vkCreateSemaphore(VulkanApp->device, &semaphoreInfo, NULL, &VulkanApp->debug_image_avail);
			VulkanApp->vkCreateSemaphore(VulkanApp->device, &semaphoreInfo, NULL, &VulkanApp->debug_render_done);
		}

		memory->is_initialized = true;
	}
}

extern "C" GAME_UPDATE_FUNCTION(GameUpdate)
{
    uint32_t imageIndex;
    VkResult result = VulkanApp->vkAcquireNextImageKHR(VulkanApp->device, VulkanApp->swapchain, UINT64_MAX, VulkanApp->debug_image_avail, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {VulkanApp->debug_image_avail};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &VulkanApp->debug_cmd_buffers[imageIndex];

	VkSemaphore signalSemaphores[] = {VulkanApp->debug_render_done};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VulkanApp->vkQueueSubmit(VulkanApp->gfx_queue, 1, &submitInfo, VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &VulkanApp->swapchain;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	VulkanApp->vkQueuePresentKHR(VulkanApp->present_queue, &presentInfo);
}
