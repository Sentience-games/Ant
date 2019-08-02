#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

#undef near
#undef far

#include "ant_shared.h"
#include "game.h"
#include "ant_platform.h"

struct Win32_Game_Code
{
	FILETIME timestamp;
	HMODULE module;
	bool is_valid;
    
    game_init_function* GameInit;
	game_update_and_render_function* GameUpdateAndRender;
};