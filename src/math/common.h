#pragma once

#define PI32  3.1415926535f
#define TAU32 6.2831853071f

#define DEGREES(X) (((F32)(X) * PI32) / 180.0f)

inline I64
Abs(I64 num)
{
    return num & ~(1ULL << 63);
}

// TODO(soimn): Verify this
inline F32
Abs(F32 num)
{
    *(U32*)&num &= ~(1 << 31);
    
    return num;
}

inline I64
Sign(I64 num)
{
    return (num & (1ULL << 63)) * -1;
}

// TODO(soimn): Verify this
inline F32
Sign(F32 num)
{
    return (*(U32*)&num & (1 << 31)) * -1.0f;
}

// TODO(soimn): replace this with an intrinsic
inline F32
Sqrt(F32 num, U8 precision = 5)
{
    F32 result = num / 2.0f;
    
    F32 abs_num = Abs(num);
    U32 num_iterations = (abs_num < 10.0f ? precision : (abs_num < 1000.0f ? 2 * precision : 4 * precision));
    
    for (U32 i = 0; i < num_iterations; ++i)
    {
        result = result - (result * result - num) / (2.0f * result);
    }
    
    return result;
}