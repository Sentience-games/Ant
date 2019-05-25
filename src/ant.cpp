#include "ant.h"

Platform_API_Functions* Platform;

/*
inline void
UpdateAndRenderAllEntities(World_Chunk** chunks, U32 chunk_count)
{
    for (World_Chunk* chunk = *chunks, chunk < *chunks + chunk_count; chunk = *(chunks + (chunk - *chunks) + 1))
    {
            for (Entity* e : chunk->entities)
        {
    U32 component = 0;
while (component = EntityScanComponentFlag(e->component_flag), component != UINT32_MAX)
{
switch(component)
{
// case COMPONENT_ID: UpdateCOMPONENT_NAME(e->COMPONENT_NAME); break;
}
}

// Run physics

// Add entity to render batch
}
}
}
*/

extern "C"
GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Platform = &memory->platform_api;
    Game_State* state = memory->state;
    
    (void) state;
    
    if (!memory->is_initialized)
    {
        LoadAllAssetFiles(&state->assets, &state->asset_arena);
    }
}