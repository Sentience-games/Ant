#pragma once

#define APPLICATION_NAME "ant"
#define APPLICATION_VERSION 0

#include "ant_platform.h"

#include "ant_memory.h"

#include "assets/assets.h"
#include "assets/assets.cpp"

struct Game_State
{
    Memory_Arena persistent_memory;
    Memory_Arena frame_local_memory;
    Memory_Arena asset_arena;
    
    Game_Assets assets;
};