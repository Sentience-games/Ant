#include "ant.h"

extern "C"
GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Platform = &memory->platform_api;
    Game_State* state = memory->state;
    
    if (!state->is_initialized)
    {
        Error_Stream error_stream = BeginErrorStream(1024);
        ReloadAssets(&state->assets, &error_stream);
        LogErrorStream(&error_stream);
        EndErrorStream(&error_stream);
        
        state->is_initialized = true;
    }
    
    switch (state->game_mode)
    {
        case MAIN_MENU:
        // TODO(soimn): Update and render main menu
        break;
        
        case GAME:
        if (state->is_paused)
        {
            // TODO(soimn): Update and render pause menu
        }
        
        else
        {
            // TODO(soimn): Simulate game world
        }
        break;
    }
}