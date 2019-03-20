#pragma once

#include "ant_platform.h"

#include "utils/memory_utils.h"

struct game_state
{
    bool initialized;
    
    Memory_Arena persistent_memory;
    Memory_Arena frame_local_memory;
};