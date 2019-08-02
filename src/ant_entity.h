#pragma once

#include "ant_shared.h"

#include "utils/transform.h"
#include "utils/bounding_volumes.h"

struct Entity
{
    Transform transform;
    Bounding_Sphere bounding_sphere;
};