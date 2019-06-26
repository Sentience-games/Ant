#include "ant.h"

Platform_API_Functions* Platform;

extern "C"
GAME_UPDATE_AND_RENDER_FUNCTION(GameUpdateAndRender)
{
    Platform = &memory->platform_api;
    Game_State* state = memory->state;
    
    (void) state;
}