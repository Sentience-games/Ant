#pragma once

#define APPLICATION_NAME "ant"
#define APPLICATION_VERSION 0

#include "ant_platform.h"

#include "ant_memory.h"

#include "assets/assets.h"
#include "assets/assets.cpp"

#include "utils/error_stream.h"

#include "world/entity.h"

enum GAME_MODE
{
    MAIN_MENU,
    GAME,
};

struct Game_State
{
    Memory_Arena persistent_memory;
    Memory_Arena frame_local_memory;
    Memory_Arena asset_arena;
    
    Game_Assets assets;
    
    Enum8(GAME_MODE) game_mode;
    
    bool is_initialized;
    bool is_paused;
};

// TODO(soimn): Investigate if this would cause any trouble or avoidable mistakes
Platform_API_Functions* Platform;