#include "ant.h"

EXPORT GAME_INIT_FUNCTION(GameInit)
{
	bool succeeded = false;

	Platform = &memory->platform_api;
	Renderer = &memory->platform_api.RendererAPI;
	RendererState = memory->platform_api.RendererAPI.RendererState;
	VulkanAPI = memory->platform_api.RendererAPI.VulkanAPI;

	DebugArena = &memory->debug_arena;

	if (!memory->is_initialized)
	{
		VulkanApp = PushStruct(DebugArena, vulkan_app);

		/// SWAPCHAIN
		Renderer->CreateSwapchain(NULL);
		Renderer->CreateSwapchainImages(NULL);

		/// RENDER PASS

		vulkan_render_pass_create_info pass_info = {};
		pass_info.use_default_attachment = true;
		pass_info.use_default_subpass	 = true;
		VulkanApp->debug_render_pass = Renderer->CreateRenderPass(pass_info);

		/// GRAPHICS PIPELINE LAYOUT
		VulkanApp->debug_pipeline_layout = Renderer->CreatePipelineLayout(NULL, 0, NULL, 0);

		/// GRAPHICS PIPELINE CREATION
		vulkan_graphics_pipeline_create_info pip_create_info = {1, true};
		pip_create_info.render_pass		= &VulkanApp->debug_render_pass;
		pip_create_info.pipeline_layout = &VulkanApp->debug_pipeline_layout;

		vulkan_shader_stage_create_info shader_stages = {};
		platform_file_handle vert = Platform->DebugReadFile("a:\\build\\res\\vert.spv", alignof(uint32));
		platform_file_handle frag = Platform->DebugReadFile("a:\\build\\res\\frag.spv", alignof(uint32));
		shader_stages.vertex_shader	  = Renderer->CreateShaderModule((uint32) vert.size, (uint32*) vert.data, NULL, 0);
		shader_stages.fragment_shader = Renderer->CreateShaderModule((uint32) frag.size, (uint32*) frag.data, NULL, 0);

		VkPipelineShaderStageCreateInfo shader_stage_create_infos [2] = {};
		Renderer->CreateShaderStageInfos(shader_stages, shader_stage_create_infos, 2);
		uint32 stage_count = ARRAY_COUNT(shader_stage_create_infos);
		pip_create_info.shader_stage_count = &stage_count;
		pip_create_info.shader_stage_info = shader_stage_create_infos;
		Renderer->CreateGraphicsPipelines(pip_create_info, &VulkanApp->debug_pipeline, 1);

		/// FRAMEBUFFER CREATION
		{
			VulkanApp->debug_framebuffer_count = RendererState->swapchain.image_count;
			VulkanApp->debug_framebuffers = PushArray(DebugArena, VkFramebuffer, RendererState->swapchain.image_count);

			for (uint32 i = 0; i < VulkanApp->debug_framebuffer_count; ++i)
			{
				vulkan_framebuffer_create_info create_info = {};
				create_info.attachment_count = 1;
				create_info.attachments		 = &RendererState->swapchain.image_views[i];
				create_info.extent			 = RendererState->swapchain.extent;
				create_info.layers			 = 1;

				VulkanApp->debug_framebuffers[i] = Renderer->CreateFramebuffer(create_info, VulkanApp->debug_render_pass);
			}
		}

		/// COMMAND POOL CREATION
		VulkanApp->debug_cmd_buffer_count = VulkanApp->debug_framebuffer_count;
		VulkanApp->debug_cmd_pool = Renderer->CreateCommandPool(RendererState->graphics_family, 0, NULL);

		/// COMMAND BUFFER ALLOCATION
		VkCommandBufferAllocateInfo buffer_alloc_info = {};
		buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buffer_alloc_info.commandPool = VulkanApp->debug_cmd_pool;
		buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		buffer_alloc_info.commandBufferCount = VulkanApp->debug_cmd_buffer_count;

		VulkanAPI->vkAllocateCommandBuffers(RendererState->device, &buffer_alloc_info, VulkanApp->debug_cmd_buffers);

		for (size_t i = 0; i < VulkanApp->debug_cmd_buffer_count; i++) {

			/// BEGIN COMMAND BUFFER RECORDING
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			VulkanAPI->vkBeginCommandBuffer(VulkanApp->debug_cmd_buffers[i], &beginInfo);

			/// SPECIFY THE RENDERPASS COMPATIBILITY
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = VulkanApp->debug_render_pass;
			renderPassInfo.framebuffer = VulkanApp->debug_framebuffers[i];
			renderPassInfo.renderArea.offset = {0, 0};
			renderPassInfo.renderArea.extent = RendererState->swapchain.extent;
			VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			/// COMMAND BUFFER RECORDING
			VulkanAPI->vkCmdBeginRenderPass(VulkanApp->debug_cmd_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			VulkanAPI->vkCmdBindPipeline(VulkanApp->debug_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanApp->debug_pipeline);
			VulkanAPI->vkCmdDraw(VulkanApp->debug_cmd_buffers[i], 3, 1, 0, 0);
			VulkanAPI->vkCmdEndRenderPass(VulkanApp->debug_cmd_buffers[i]);
			VulkanAPI->vkEndCommandBuffer(VulkanApp->debug_cmd_buffers[i]);
		}

		/// SEMAPHORE CREATION
		VulkanApp->debug_image_avail = Renderer->CreateSemaphore(NULL, 0);
		VulkanApp->debug_render_done = Renderer->CreateSemaphore(NULL, 0);

		memory->is_initialized = true;
	}

	// TODO(soimn): refactor in order to maintain the ability to properly
	//				hot reload without asserting due to this being set only at init time
	succeeded = true;

	return succeeded;
}

EXPORT GAME_UPDATE_FUNCTION(GameUpdate)
{
    uint32_t imageIndex;
    VulkanAPI->vkAcquireNextImageKHR(RendererState->device, RendererState->swapchain.handle, UINT64_MAX, VulkanApp->debug_image_avail, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &VulkanApp->debug_image_avail;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &VulkanApp->debug_cmd_buffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &VulkanApp->debug_render_done;

	VulkanAPI->vkQueueSubmit(RendererState->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &VulkanApp->debug_render_done;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &RendererState->swapchain.handle;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	VulkanAPI->vkQueuePresentKHR(RendererState->present_queue, &presentInfo);
}

EXPORT GAME_CLEANUP_FUNCTION(GameCleanup)
{
}
