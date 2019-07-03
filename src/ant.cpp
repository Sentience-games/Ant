#include "ant.h"

Platform_API_Functions* Platform;

extern "C"
GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Platform = &memory->platform_api;
    Game_State* state = memory->state;
    
    if (!state->is_initialized)
    {
        Error_Stream error_stream = BeginErrorStream(1024);
        ReloadAssets(&state->assets, &error_stream);
        EndErrorStream(&error_stream);
        
        state->is_initialized = true;
    }
}