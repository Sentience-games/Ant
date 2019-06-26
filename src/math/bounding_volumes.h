#pragma once

#include "math/vector.h"

struct Region3D
{
    V3 position;
    V3 dimensions;
};

struct Region2D
{
    V2 position;
    V2 dimensions;
};

typedef Region3D AABB;

struct Bounding_Sphere
{
    V3 position;
    F32 radius;
};