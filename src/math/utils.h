#pragma once

#include "ant_shared.h"

// TODO(soimn): switch to intrinsic
inline f32
Floor(f32 num)
{
    Assert((f32) UINT64_MAX > num);
    
    f32 result = (f32) ((u64) num);
    
    return result;
}

// TODO(soimn): switch to intrinsic
inline f32
Ceil(f32 num)
{
    Assert((f32) UINT64_MAX > num);
    
    f32 result  = num;
    f32 floored = (f32) ((u64) num);
    
    result = (result == floored ? result : floored + 1.0f);
    
    return result;
}