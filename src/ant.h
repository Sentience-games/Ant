#pragma once

#include "utils/fixed_int.h"

struct game_memory {
	bool is_initialized;

	void* persistent_memory;
	void* transient_memory;
	uint64 persistent_size;
	uint64 transient_size;
};
