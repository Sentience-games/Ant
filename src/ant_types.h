#pragma once

#include <stdint.h>

typedef int8_t	int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t	 uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int_fast8_t	 fast_int8;
typedef int_fast16_t fast_int16;
typedef int_fast32_t fast_int32;
typedef int_fast64_t fast_int64;

typedef uint_fast8_t  fast_uint8;
typedef uint_fast16_t fast_uint16;
typedef uint_fast32_t fast_uint32;
typedef uint_fast64_t fast_uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef int8  i8;
typedef int16 i16;
typedef int32 i32;
typedef int64 i64;

typedef uint8  u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef float  f32;
typedef double f64;

typedef u32 b32;

#define enum8(type)  u8
#define enum16(type) u16
#define enum32(type) u32
#define enum64(type) u64

#ifdef USE_32BIT_POINTER_TYPES
typedef u32 memory_index;
#define MEMORY_INDEX_MAX UINT32_MAX
#else
typedef u64 memory_index;
#define MEMORY_INDEX_MAX UINT64_MAX
#endif

struct buffer
{
	memory_index size;
	u8* data;
};

typedef buffer string;

#define CONST_STRING(STR) { sizeof(STR) - 1, (u8*)(STR) }
