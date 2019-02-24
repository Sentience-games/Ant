#pragma once

#include "ant_platform.h"

#include "utils/utility_defines.h"
#include "utils/memory_utils.h"
#include "ant_types.h"
#include "utils/cstring.h"

global_variable platform_api_functions* Platform;
global_variable memory_arena* DebugArena;

// TODO(soimn):
/*
 * - Clean up all renderer calls in Ant.cpp
 * - Architecture a flexible and robust entity system with an
 *	  easy to use api for instantiation and parameter tweaking
 */
