#include "game.h"

GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Game_State* state = game_memory->state;
    Platform          = &game_memory->platform_api;
    
    if (!state->is_initialized)
    {
        state->is_initialized = true;
    }
}