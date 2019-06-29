#pragma once

#include "ant_shared.h"

// TODO(soimn): switch to intrinsic
inline I32
Abs(I32 num)
{
    return (num < 0 ? -num : num);
}

// TODO(soimn): switch to intrinsic
inline F32
Abs(F32 num)
{
    return (num < 0 ? -num : num);
}

// TODO(soimn): switch to intrinsic
inline F32
Floor(F32 num)
{
    Assert((F32) UINT64_MAX > num);
    
    F32 result = (F32) ((U64) num);
    
    return result;
}

// TODO(soimn): switch to intrinsic
inline F32
Ceil(F32 num)
{
    Assert((F32) UINT64_MAX > num);
    
    F32 result  = num;
    F32 floored = (F32) ((U64) num);
    
    result = (result == floored ? floored : floored + 1.0f);
    
    return result;
}