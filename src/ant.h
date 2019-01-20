#pragma once

#include "utils/fixed_int.h"

struct game_memory {
	bool is_initialized;

	void* persistent_memory;
	uint8* persistent_stack_ptr;
	void* transient_memory;
	uint8* transient_stack_ptr;
	uint64 persistent_size;
	uint64 transient_size;
};
