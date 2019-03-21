#pragma once

#include "ant_shared.h"

inline U32
Square(U8 n)
{
	return ((U32)(n) * (U32)(n));
}

inline F32
Square(F32 n)
{
	return n * n;
}

// TODO(soimn): replace this with an intrinsic
inline F32
Sqrt(F32 num, U8 precision = 5)
{
    F32 result = num / 2.0f;
    
    // TODO(soimn): see if there is a better solution
    U32 num_iterations = (num < 10.0f ? precision : (num < 1000.0f ? 2 * precision : 4 * precision));
    
    for (U32 i = 0; i < num_iterations; ++i)
    {
        result = result - (Square(result) - num) / (2.0f * result);
    }
    
    return result;
}

inline u64 Pow2(U8 exp);

inline u64
Pow(u64 num, U8 exp)
{
    Assert(exp < 64);
    
    u64 result = 1;
    
    if (num == 2 && exp <= 6)
    {
        result = Pow2(exp);
    }
    
    else
    {
        for (U32 i = 0; i < exp; ++i)
        {
            result *= exp;
        }
    }
    
    return result;
}

inline u64
Pow2(U8 exp)
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