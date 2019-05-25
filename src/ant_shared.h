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
#define LARGE_INT_LOW(number) (((U64)(number)  >> 0)  & UINT32_MAX)
#define LARGE_INT_HIGH(number) (((U64)(number) >> 32) & UINT32_MAX)
#define ASSEMBLE_LARGE_INT(high, low) (U64)(((U64)(high) << 32) | ((U64)(low) << 0))
#define U32_FROM_BYTES(a, b, c, d) (((U32)(a) << 24) | ((U32)(b) << 16) | ((U32)(c) << 8) | ((U32)(d) << 0))
#define ENDIAN_SWAP32(n) ((((U32) (n) & 0xFF000000) >> 24) | (((U32) (n) & 0x00FF0000) >> 8) | (((U32) (n) & 0x0000FF00) << 8) | (((U32) (n) & 0x000000FF) << 24))

#define SERVER_ADDRESS(a, b, c, d) U32_FROM_BYTES(a, b, c, d)
#define LOCAL_HOST SERVER_ADDRESS(127, 0, 0, 1)

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define BITS(num) (1 << (num))
#define ISBITSET(flag, bit) (((flag) & (bit)) != 0)

#define OFFSETOF(obj, var) (Uintptr) &(((obj*)0)->var)

#define INVALID_CODE_PATH Assert(!"Invalid code path")
#define INVALID_DEFAULT_CASE default: INVALID_CODE_PATH; break
#define NOT_IMPLEMENTED Assert(!"Not implemented");

#define BREAK_ON_FALSE(boolean) if ((boolean)); else break

#define MAKE_VERSION(major, minor, patch)\
(((major) << 22) | ((minor) << 12) | (patch))

#include "ant_types.h"
#include "utils/assert.h"