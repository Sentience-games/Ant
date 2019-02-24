#include "ant.h"

EXPORT GAME_INIT_FUNCTION(GameInit)
{
	bool succeeded = false;

	Platform = &memory->platform_api;

	DefaultMemoryArenaAllocationRoutines = memory->default_allocation_routines;
	DebugArena = &memory->debug_arena;

	if (!memory->is_initialized)
	{
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
}

EXPORT GAME_CLEANUP_FUNCTION(GameCleanup)
{
}
