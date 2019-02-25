#include "ant.h"

EXPORT GAME_INIT_FUNCTION(GameInit)
{
	bool succeeded = false;

	Platform = &memory->platform_api;

	DefaultMemoryArenaAllocationRoutines = memory->default_allocation_routines;
	DebugArena = &memory->debug_arena;

	bool failed_memory_initiaization = false;
	if (!memory->is_initialized)
	{
// 		platform_file_group file_group = Platform->GetAllFilesOfTypeBegin(PlatformFileType_ShaderFile);
// 
// 		VkShaderModule vertex_shader   = {};
// 		VkShaderModule frag_shader	   = {};
// 		VkShaderModule geometry_shader = {};
// 
// 		do
// 		{
// 			// NOTE(soimn): the version running from nvim crashes at this line, nullptr dereference?
// 			if (strcompare(file_group.first_file_info->base_name, "polygon_v"));
// 			if (strcompare(file_group.first_file_info->base_name, "polygon_f"));
// 			if (strcompare(file_group.first_file_info->base_name, "polygon_g"));
// 			break;
// 		}
// 
// 		while(vertex_shader == VK_NULL_HANDLE && frag_shader == VK_NULL_HANDLE && geometry_shader == VK_NULL_HANDLE);
// 
// 		Platform->GetAllFilesOfTypeEnd(&file_group);

		memory->is_initialized = true;
	}

	succeeded = !failed_memory_initiaization;

	return succeeded;
}


// TODO(soimn): find out how to keep track of command submission and frames in flight
EXPORT GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
}

EXPORT GAME_CLEANUP_FUNCTION(GameCleanup)
{
}
