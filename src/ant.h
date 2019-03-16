#pragma once

#include "ant_platform.h"

global_variable platform_api_functions* Platform;

global_variable platform_game_input* NewInput;
global_variable platform_game_input* OldInput;

global_variable memory_arena* FrameTempArena;
global_variable memory_arena* DebugArena;

#include "immediate/rendering.h"
#include "editor/gui.h"
