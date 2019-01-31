#pragma once

#include "ant_platform.h"

#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "utils/fixed_int.h"

// TODO(soimn): implement proper assertions for use in game code
#include "utils/assert.h"

global_variable vulkan_application* VulkanApp;
global_variable platform_api_functions* Platform;

global_variable memory_arena* DebugArena;
