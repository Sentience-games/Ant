#include "ant.h"

Platform_API_Functions* Platform;

extern "C"
GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Platform = &memory->platform_api;
    Game_State* state = memory->state;
    
    if (!state->initialized)
    {
        state->persistent_memory.current_block  = Platform->AllocateMemoryBlock(MEGABYTES(64));
        ++state->persistent_memory.block_count;
        state->frame_local_memory.current_block = Platform->AllocateMemoryBlock(KILOBYTES(1));
        ++state->frame_local_memory.block_count;
        
        state->initialized = true;
    }
}
