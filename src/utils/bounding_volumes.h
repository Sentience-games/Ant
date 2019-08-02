#pragma once

#include "math/vector.h"

struct Bounding_Sphere
{
    V3 position;
    F32 radius;
};

struct Swept_Bounding_Sphere
{
    V3 p0;
    V3 p1;
    F32 radius;
};