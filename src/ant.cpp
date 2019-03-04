#include "ant.h"

EXPORT GAME_INIT_FUNCTION(GameInit)
{
	bool succeeded = false;

	Platform = &memory->platform_api;

	VulkanAPI   = &memory->vulkan_api;
	VulkanState = &memory->vulkan_state;

	DefaultMemoryArenaAllocationRoutines = memory->default_allocation_routines;
	FrameTempArena = &memory->frame_temp_arena;
	DebugArena	   = &memory->debug_arena;

	NewInput = &memory->new_input;
	OldInput = &memory->old_input;

	bool failed_memory_initiaization = false;
	if (!memory->is_initialized)
	{

		// TODO(soimn): set this to the current windows aspect ratio
		VulkanState->render_target.immediate_viewport_dimensions = {16.0f, 9.0f};

		memory->is_initialized = true;
	}

	succeeded = !failed_memory_initiaization;

	return succeeded;
}

EXPORT GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
	// TODO(soimn); consider waiting on fence before calling this
	VulkanAPI->vkResetCommandPool(VulkanState->device, VulkanState->render_target.render_pool, 0);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext			= NULL;
	begin_info.flags			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VulkanAPI->vkBeginCommandBuffer(VulkanState->render_target.render_buffer, &begin_info);

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = VulkanState->render_target.render_pass;
	render_pass_info.framebuffer = VulkanState->render_target.framebuffer;
	render_pass_info.renderArea.extent = VulkanState->swapchain.extent;

	VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	VulkanAPI->vkCmdBeginRenderPass(VulkanState->render_target.render_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	UIRender();
	
	RenderImmediateCalls(VulkanState->render_target.render_buffer);

	VulkanAPI->vkCmdEndRenderPass(VulkanState->render_target.render_buffer);
	VulkanAPI->vkEndCommandBuffer(VulkanState->render_target.render_buffer);

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = waitStages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &VulkanState->render_target.render_buffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &VulkanState->render_target.render_done_semaphore;

	VulkanAPI->vkQueueSubmit(VulkanState->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
}

EXPORT GAME_CLEANUP_FUNCTION(GameCleanup)
{
}
