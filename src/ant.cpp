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
// 		platform_file_group file_group = Platform->GetAllFilesOfTypeBegin(PlatformFileType_AssetFile);
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
