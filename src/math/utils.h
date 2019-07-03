#pragma once

#include "ant_shared.h"

// TODO(soimn): switch to intrinsic
inline I32
Abs(I32 num)
{
    return (num < 0 ? -num : num);
}

// TODO(soimn): switch to intrinsic
inline I64
Abs(I64 num)
{
    return (num < 0 ? -num : num);
}

// TODO(soimn): switch to intrinsic
inline F32
Abs(F32 num)
{
    return (num < 0 ? -num : num);
}

inline I32
Sign(I32 n)
{
    return ((n & BITS(31)) != 0 ? -1 : 1);
}

inline I64
Sign(I64 n)
{
    return ((n & BITS(63)) != 0 ? -1 : 1);
}

// TODO(soimn): switch to intrinsic
inline F32
Floor(F32 num)
{
    Assert((F32) U64_MAX > num);
    
    F32 result = (F32) ((U64) num);
    
    return result;
}

// TODO(soimn): switch to intrinsic
inline F32
Ceil(F32 num)
{
    Assert((F32) U64_MAX > num);
    
    F32 result  = num;
    F32 floored = (F32) ((U64) num);
    
    result = (result == floored ? floored : floored + 1.0f);
    
    return result;
}