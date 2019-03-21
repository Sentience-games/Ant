#pragma once

#include "ant_platform.h"

#include "ant_memory.h"

struct Game_State
{
    bool initialized;
    
    Memory_Arena persistent_memory;
    Memory_Arena frame_local_memory;
};