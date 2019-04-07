#pragma once

#include "ant_types.h"
#include "math/vector.h"
#include "math/matrix.h"

struct Transform
{
    V3 position;
    Quat rotation; // Rotation needed to go from a basis vector to the current heading
};