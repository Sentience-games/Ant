#pragma once

#define APPLICATION_NAME "ant"
#define APPLICATION_VERSION 0

#include "ant_platform.h"

#include "ant_memory.h"

struct Game_State
{
    bool initialized;
    
    Memory_Arena persistent_memory;
    Memory_Arena frame_local_memory;
};