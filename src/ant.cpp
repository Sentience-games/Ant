#include "ant.h"

EXPORT GAME_INIT_FUNCTION(GameInit)
{
	bool succeeded = false;
    
	Platform = &memory->platform_api;
    
	Vulkan = &memory->vulkan_state;
    
	DefaultMemoryArenaAllocationRoutines = memory->default_allocation_routines;
	FrameTempArena = &memory->frame_temp_arena;
	DebugArena	   = &memory->debug_arena;
    
	NewInput = &memory->new_input;
	OldInput = &memory->old_input;
    
	bool failed_memory_initiaization = false;
	if (!memory->is_initialized)
	{
        
		// TODO(soimn): set this to the current windows aspect ratio
		Vulkan->render_target.immediate_viewport_dimensions = {16.0f, 9.0f};
        
		memory->is_initialized = true;
	}
    
	succeeded = !failed_memory_initiaization;
    
	return succeeded;
}

bool category_open    = false;
bool category_open2   = false;

// TODO(soimn): move all direct communication with the rendering api, such that this file may be API agnostic
EXPORT GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
	UIBeginPanel(UIPanel_Debug, "Debug", {5.5f, 2.0f}, 5.0f, 1.0f);
	UIButton("Label", UIElementWidth_Half);
	UIButton("Label", UIElementWidth_Half);
	UINewRow();
	if (UICategoryButton("NewCategory", UIElementWidth_Full, &category_open))
	{
		UINewRow();
		UINewIndent();
		UIButton("Label", UIElementWidth_Half);
		UINewRow();
		UIButton("Label", UIElementWidth_Full);
		UINewRow();
        
		if (UICategoryButton("NewCategory", UIElementWidth_Full, &category_open2))
		{
			UINewRow();
			UINewIndent();
			UIButton("Label", UIElementWidth_Half);
			UINewRow();
			UIButton("Label", UIElementWidth_Full);
			UINewRow();
			UIButton("Label", UIElementWidth_Eighth);
			UIRemoveIndent();
		}
        
		UINewRow();
        
		UIButton("Label", UIElementWidth_Quarter);
		UIRemoveIndent();
	}
	UIEndPanel();
    
	// TODO(soimn); consider waiting on fence before calling this
	Vulkan->vkResetCommandPool(Vulkan->device, Vulkan->render_target.render_pool, 0);
    
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext			= NULL;
	begin_info.flags			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
	Vulkan->vkBeginCommandBuffer(Vulkan->render_target.render_buffer, &begin_info);
    
	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = Vulkan->render_target.render_pass;
	render_pass_info.framebuffer = Vulkan->render_target.framebuffer;
	render_pass_info.renderArea.extent = Vulkan->swapchain.extent;
    
	VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;
    
	Vulkan->vkCmdBeginRenderPass(Vulkan->render_target.render_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
	RenderImmediateCalls(Vulkan->render_target.render_buffer);
    
	Vulkan->vkCmdEndRenderPass(Vulkan->render_target.render_buffer);
	Vulkan->vkEndCommandBuffer(Vulkan->render_target.render_buffer);
    
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = waitStages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &Vulkan->render_target.render_buffer;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &Vulkan->render_target.render_done_semaphore;
    
	Vulkan->vkQueueSubmit(Vulkan->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
}

EXPORT GAME_CLEANUP_FUNCTION(GameCleanup)
{
}
