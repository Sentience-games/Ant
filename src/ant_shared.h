#pragma once

/// 
/// Shared definitions
/// 

#ifdef ANT_DEBUG
#define ANT_ENABLE_ASSERT
#define ANT_ENABLE_HOT_RELOAD
#endif

#define internal static
#define global static
#define local_persist static

#define UNUSED_PARAMETER(x) (void)(x)
#define ARRAY_COUNT(array) (sizeof((array)) / sizeof((array)[0]))

#define BYTES(N) (N)
#define KILOBYTES(N) (BYTES(N) * 1024ULL)
#define MEGABYTES(N) (KILOBYTES(N) * 1024ULL)
#define GIGABYTES(N) (MEGABYTES(N) * 1024ULL)
#define TERRABYTES(N) (GIGABYTES(N) * 1024ULL)

#define MILLISECONDS(millis_to_be_converted_to_seconds) (((F32) (millis_to_be_converted_to_seconds)) / 1000.0f)

#define BITS(num) ((U64)1 << (num))
#define ISBITSET(flag, bit) (((flag) & (bit)) != 0)

#define OFFSETOF(obj, var) (UMM) &(((obj*)0)->var)

#define INVALID_CODE_PATH Assert(!"Invalid code path")
#define INVALID_DEFAULT_CASE default: INVALID_CODE_PATH; break
#define NOT_IMPLEMENTED Assert(!"Not implemented");

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)

#ifdef ANT_ENABLE_ASSERT
#define Assert(EX) if (EX); else *(volatile int*) 0 = 0
#else
#define Assert(EX)
#endif

#define StaticAssert(EX, ...) static_assert(EX, ##__VA_ARGS__)

#include "ant_types.h"

enum GET_OR_SET
{
    GET = 0,
    SET = 1,
};

enum FORWARD_OR_BACKWARD
{
    FORWARD  = 0,
    BACKWARD = 1,
};

/// 
/// Shared utility functions
/// 

inline U16
SwapEndianess(U16 num)
{
    return ((num & 0xFF00) >> 8) | ((num & 0x00FF) << 8);
}

inline U32
SwapEndianess(U32 num)
{
    U32 lower  = (U32) SwapEndianess((U16)((num & 0xFFFF0000) >> 16));
    U32 higher = (U32) (SwapEndianess((U16)(num & 0x0000FFFF))) << 16;
    
    return higher | lower;
}

inline U64
SwapEndianess(U64 num)
{
    U64 lower  = (U64) SwapEndianess((U32)((num & 0xFFFFFFFF00000000) >> 32));
    U64 higher = (U64) (SwapEndianess((U32)(num & 0x00000000FFFFFFFF))) << 32;
    
    return higher | lower;
}

inline U8
Max(U8 n0, U8 n1)
{
    return (n0 > n1 ? n0 : n1);
}

inline U16
Max(U16 n0, U16 n1)
{
    return (n0 > n1 ? n0 : n1);
}

inline U32
Max(U32 n0, U32 n1)
{
    return (n0 > n1 ? n0 : n1);
}

inline U64
Max(U64 n0, U64 n1)
{
    return (n0 > n1 ? n0 : n1);
}

inline U8
Min(U8 n0, U8 n1)
{
    return (n0 < n1 ? n0 : n1);
}

inline U16
Min(U16 n0, U16 n1)
{
    return (n0 < n1 ? n0 : n1);
}

inline U32
Min(U32 n0, U32 n1)
{
    return (n0 < n1 ? n0 : n1);
}

inline U64
Min(U64 n0, U64 n1)
{
    return (n0 < n1 ? n0 : n1);
}

inline F32
Max(F32 n0, F32 n1)
{
    return (n0 > n1 ? n0 : n1);
}

inline F32
Min(F32 n0, F32 n1)
{
    return (n0 < n1 ? n0 : n1);
}