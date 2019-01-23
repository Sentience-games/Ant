#pragma once

#include "utils/fixed_int.h"

#define APPLICATION_NAME "ant"
#define APPLICATION_VERSION 0

struct game_memory
{
	bool is_initialized;

	void* persistent_memory;
	void* transient_memory;
	uint64 persistent_size;
	uint64 transient_size;
};

#define GAME_INIT_FUNCTION(name) void name (game_memory* memory)
#define GAME_UPDATE_FUNCTION(name) void name (game_memory* memory, float delta_t)
