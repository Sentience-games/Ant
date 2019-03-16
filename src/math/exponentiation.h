#pragma once

#include "ant_types.h"

inline u32
Square(u8 n)
{
	return ((u32)(n) * (u32)(n));
}

inline f32
Square(f32 n)
{
	return n * n;
}

// TODO(soimn): replace this with an intrinsic
inline f32
Sqrt(f32 num, u8 precision = 5)
{
    f32 result = num / 2.0f;
    
    // TODO(soimn): see if there is a better solution
    u32 num_iterations = (num < 10.0f ? precision : (num < 1000.0f ? 2 * precision : 4 * precision));
    
    for (u32 i = 0; i < num_iterations; ++i)
    {
        result = result - (Square(result) - num) / (2.0f * result);
    }
    
    return result;
}

inline u64 Pow2(u8 exp);

inline u64
Pow(u64 num, u8 exp)
{
    Assert(exp < 64);
    
    u64 result = 1;
    
    if (num == 2 && exp <= 6)
    {
        result = Pow2(exp);
    }
    
    else
    {
        for (u32 i = 0; i < exp; ++i)
        {
            result *= exp;
        }
    }
    
    return result;
}

inline u64
Pow2(u8 exp)
{
    Assert(exp < 64);
    
    u64 result = 0;
    
    switch(exp)
    {
        case 0:
        result = 1;
        break;
        
        case 1:
        result = 2;
        break;
        
        case 2:
        result = 4;
        break;
        
        case 3:
        result = 8;
        break;
        
        case 4:
        result = 16;
        break;
        
        case 5:
        result = 32;
        break;
        
        case 6:
        result = 64;
        break;
        
        default:
        result = Pow(2, exp);
        break;
    }
    
    Assert(result != 0);
    
    return result;
}
