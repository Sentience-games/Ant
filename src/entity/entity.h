#pragma once

#include "ant_types.h"

#define ENTITY_COMPONENT_FLAG_SIZE 64

struct Entity
{
    U32 component_flag[MAX(ENTITY_COMPONENT_FLAG_SIZE / 32 + !!(ENTITY_COMPONENT_FLAG_SIZE % 32), 1)];
    
    // Required components
    Transform transform;
    Bounding_Sphere bounding_sphere;
    Asset* mesh;
    
    // Optional
    // ...
};