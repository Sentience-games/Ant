#pragma once

#define internal static
#define global static
#define global_variable static
#define local_persist static

#define UNUSED_PARAMETER(x) (void)(x)
#define ARRAY_COUNT(array) (sizeof((array)) / sizeof((array)[0]))

#define BYTES(N) (N)
#define KILOBYTES(N) (BYTES(N) * 1024ULL)
#define MEGABYTES(N) (KILOBYTES(N) * 1024ULL)
#define GIGABYTES(N) (MEGABYTES(N) * 1024ULL)
#define TERRABYTES(N) (GIGABYTES(N) * 1024ULL)

#define MAX(x, y) ((x) < (y) ? (y) : (x))
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define CLAMP(min, x, max) ((x) < min ? min : (max < (x) ? max : (x)))
#define LARGE_INT_LOW(number) (((U64)(number)  >> 0)  & U32_MAX)
#define LARGE_INT_HIGH(number) (((U64)(number) >> 32) & U32_MAX)
#define ASSEMBLE_LARGE_INT(high, low) (U64)(((U64)(high) << 32) | ((U64)(low) << 0))
#define U32_FROM_BYTES(a, b, c, d) (((U32)(a) << 24) | ((U32)(b) << 16) | ((U32)(c) << 8) | ((U32)(d) << 0))

#define SERVER_ADDRESS(a, b, c, d) U32_FROM_BYTES(a, b, c, d)
#define LOCAL_HOST SERVER_ADDRESS(127, 0, 0, 1)

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define BITS(num) (1ll << (num))
#define ISBITSET(flag, bit) (((flag) & (bit)) != 0)

#define ROUNDTO(num_to_be_rounded, num) (((num_to_be_rounded) + ((num) - 1)) & ~((num) - 1))

#define OFFSETOF(obj, var) (UMM) &(((obj*)0)->var)

#define INVALID_CODE_PATH Assert(!"Invalid code path")
#define INVALID_DEFAULT_CASE default: INVALID_CODE_PATH; break
#define NOT_IMPLEMENTED Assert(!"Not implemented");

#define BREAK_ON_FALSE(boolean) if ((boolean)); else break

#include "ant_types.h"
#include "utils/assert.h"

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

struct Asset_Tag
{
    U16 value;
};

struct Asset_ID
{
    U32 value;
};