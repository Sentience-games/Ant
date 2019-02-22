#include "ant.h"

global_variable vertex vertecies[] = {{{-0.5f, -0.5f, 0.0f}}, {{0.5f, -0.5f, 0.0f}}, {{0.5f, 0.5f, 0.0f}}, {{-0.5f, 0.5f, 0.0f}}};
global_variable uint32 indecies[] = {0, 1, 2, 2, 3, 0};

EXPORT GAME_INIT_FUNCTION(GameInit)
{
	bool succeeded = false;

	Platform = &memory->platform_api;
	Renderer = &memory->platform_api.RendererAPI;
	RendererState = memory->platform_api.RendererAPI.RendererState;
	VulkanAPI = memory->platform_api.RendererAPI.VulkanAPI;

	DefaultMemoryArenaAllocationRoutines = memory->default_allocation_routines;
	DebugArena = &memory->debug_arena;

	if (!memory->is_initialized)
	{
// 
// 		/// GRAPHICS PIPELINE LAYOUT
// 		VulkanApp->debug_pipeline_layout = Renderer->CreatePipelineLayout(NULL, 0, NULL, 0);
// 
// 		/// GRAPHICS PIPELINE CREATION
// 		vulkan_graphics_pipeline_create_info pip_create_info = {1, true};
// 		pip_create_info.render_pass		= &VulkanApp->debug_render_pass;
// 		pip_create_info.pipeline_layout = &VulkanApp->debug_pipeline_layout;
// 
// 		vulkan_shader_stage_create_info shader_stages = {};
// // 		platform_file_handle vert = Platform->DebugReadFile("a:\\build\\res\\vert.spv", alignof(uint32));
// // 		platform_file_handle frag = Platform->DebugReadFile("a:\\build\\res\\frag.spv", alignof(uint32));
// 		shader_stages.vertex_shader	  = Renderer->CreateShaderModule((uint32) vert.size, (uint32*) vert.data, NULL, 0);
// 		shader_stages.fragment_shader = Renderer->CreateShaderModule((uint32) frag.size, (uint32*) frag.data, NULL, 0);
// 
// 		VkPipelineShaderStageCreateInfo shader_stage_create_infos [2] = {};
// 		Renderer->CreateShaderStageInfos(shader_stages, shader_stage_create_infos, 2);
// 		uint32 stage_count = ARRAY_COUNT(shader_stage_create_infos);
// 		pip_create_info.shader_stage_count = &stage_count;
// 		pip_create_info.shader_stage_info = shader_stage_create_infos;
// 		Renderer->CreateGraphicsPipelines(pip_create_info, &VulkanApp->debug_pipeline, 1);
// 
// 
// 
// 		for (size_t i = 0; i < VulkanApp->debug_cmd_buffer_count; i++) {
// 
// 			/// BEGIN COMMAND BUFFER RECORDING
// 			VkCommandBufferBeginInfo beginInfo = {};
// 			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
// 			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
// 			beginInfo.pInheritanceInfo = nullptr; // Optional
// 
// 			VulkanAPI->vkBeginCommandBuffer(VulkanApp->debug_cmd_buffers[i], &beginInfo);
// 
// 			/// SPECIFY THE RENDERPASS COMPATIBILITY
// 			VkRenderPassBeginInfo renderPassInfo = {};
// 			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
// 			renderPassInfo.renderPass = VulkanApp->debug_render_pass;
// 			renderPassInfo.framebuffer = VulkanApp->debug_framebuffers[i];
// 			renderPassInfo.renderArea.offset = {0, 0};
// 			renderPassInfo.renderArea.extent = RendererState->swapchain.extent;
// 			VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
// 			renderPassInfo.clearValueCount = 1;
// 			renderPassInfo.pClearValues = &clearColor;
// 
// 			/// COMMAND BUFFER RECORDING
// 			VulkanAPI->vkCmdBeginRenderPass(VulkanApp->debug_cmd_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
// 			VulkanAPI->vkCmdBindPipeline(VulkanApp->debug_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanApp->debug_pipeline);
// 
// 			VkDeviceMemory m;
// 			vulkan_vertex_buffer_create_info cr = {};
// 			cr.size = ARRAY_COUNT(vertecies) * sizeof(vertex);
// 			cr.vertex_data = &vertecies;
// 			Renderer->CreateVertexBuffer(cr, &m, &VulkanApp->vbuffer);
// 
// 			VkDeviceMemory mo;
// 			vulkan_index_buffer_create_info cro = {};
// 			cro.size = ARRAY_COUNT(indecies) * sizeof(uint32);
// 			cro.index_data = &indecies;
// 			Renderer->CreateIndexBuffer(cro, &mo, &VulkanApp->ibuffer);
// 
// 			VkDeviceSize offsets[] = {0};
// 			VulkanAPI->vkCmdBindVertexBuffers(VulkanApp->debug_cmd_buffers[i], 0, 1, &VulkanApp->vbuffer, offsets);
// 			VulkanAPI->vkCmdBindIndexBuffer(VulkanApp->debug_cmd_buffers[i], VulkanApp->ibuffer, 0, VK_INDEX_TYPE_UINT32);
// 			VulkanAPI->vkCmdDrawIndexed(VulkanApp->debug_cmd_buffers[i], ARRAY_COUNT(indecies), 1, 0, 0, 0);
// 			VulkanAPI->vkCmdEndRenderPass(VulkanApp->debug_cmd_buffers[i]);
// 			VulkanAPI->vkEndCommandBuffer(VulkanApp->debug_cmd_buffers[i]);
// 		}

		memory->is_initialized = true;
	}

	// TODO(soimn): refactor in order to maintain the ability to properly
	//				hot reload without asserting due to this being set only at init time
	succeeded = true;

	return succeeded;
}


// TODO(soimn): find out how to keep track of command submission and frames in flight
EXPORT GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{

// 	uint32_t imageIndex;
//     VulkanAPI->vkAcquireNextImageKHR(RendererState->device, RendererState->swapchain.handle, UINT64_MAX, VulkanApp->debug_image_avail, VK_NULL_HANDLE, &imageIndex);
// 
// 	VkSubmitInfo submitInfo = {};
// 	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
// 
// 	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
// 	submitInfo.waitSemaphoreCount = 1;
// 	submitInfo.pWaitSemaphores = &VulkanApp->debug_image_avail;
// 	submitInfo.pWaitDstStageMask = waitStages;
// 	submitInfo.commandBufferCount = 1;
// 	submitInfo.pCommandBuffers = &VulkanApp->debug_cmd_buffers[imageIndex];
// 
// 	submitInfo.signalSemaphoreCount = 1;
// 	submitInfo.pSignalSemaphores = &VulkanApp->debug_render_done;
// 
// 	VulkanAPI->vkQueueSubmit(RendererState->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
// 
// 	VkPresentInfoKHR presentInfo = {};
// 	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
// 
// 	presentInfo.waitSemaphoreCount = 1;
// 	presentInfo.pWaitSemaphores = &VulkanApp->debug_render_done;
// 
// 	presentInfo.swapchainCount = 1;
// 	presentInfo.pSwapchains = &RendererState->swapchain.handle;
// 	presentInfo.pImageIndices = &imageIndex;
// 
// 	presentInfo.pResults = nullptr;
// 
// 	VulkanAPI->vkQueuePresentKHR(RendererState->present_queue, &presentInfo);
// 	VulkanAPI->vkDeviceWaitIdle(RendererState->device);
}

EXPORT GAME_CLEANUP_FUNCTION(GameCleanup)
{
}
