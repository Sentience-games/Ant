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

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#define BITS(num) (1 << (num))
#define ISBITSET(flag, bit) (((flag) & (bit)) != 0)

#define OFFSETOF(obj, var) (uintptr) &(((obj*)0)->var)

#define INVALID_CODE_PATH Assert(!"Invalid code path")
#define INVALID_DEFAULT_CASE default: INVALID_CODE_PATH; break

#define BREAK_ON_FALSE(EX, boolean)\
boolean = EX;\
if (!(boolean)) break


#include "ant_types.h"
#include "utils/assert.h"