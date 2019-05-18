#pragma once

#include <stdint.h>

typedef intptr_t Intptr;
typedef uintptr_t Uintptr;

typedef int8_t  I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef float  F32;
typedef double F64;

typedef U32 B32;

#define Flag8(type)  U8
#define Flag16(type) U16
#define Flag32(type) U32
#define Flag64(type) U64

#define Enum8(type)  U8
#define Enum16(type) U16
#define Enum32(type) U32
#define Enum64(type) U64

#ifdef ANT_32BIT
typedef U32 UMM;
typedef I32 IMM;
#else
typedef U64 UMM;
typedef I64 IMM;
#endif

struct Buffer
{
    UMM size;
	U8* data;
};

typedef Buffer String;