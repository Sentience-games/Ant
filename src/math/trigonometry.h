#pragma once

#include <math.h>

// TODO(soimn): implement sine, cosine, tangent, inv_sine, inv_cosine, inv_tangent and friends
// TODO(soimn): @Speed

inline F32
Cos(F32 angle)
{
    return cosf(angle);
}

inline F32
Sin(F32 angle)
{
    return sinf(angle);
}

inline F32
Tan(F32 angle)
{
    return Sin(angle) / Cos(angle);
}