#pragma once

#include "math/vector.h"

struct Transform
{
    V3 position;
    Quat rotation;
    V3 scale;
};