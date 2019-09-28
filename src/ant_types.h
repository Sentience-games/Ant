#pragma once

#ifdef ANT_PLATFORM_WINDOWS
typedef signed __int8  I8;
typedef signed __int16 I16;
typedef signed __int32 I32;
typedef signed __int64 I64;

typedef unsigned __int8  U8;
typedef unsigned __int16 U16;
typedef unsigned __int32 U32;
typedef unsigned __int64 U64;
#else
typedef __INT8_TYPE__  I8;
typedef __INT16_TYPE__ I16;
typedef __INT32_TYPE__ I32;
typedef __INT64_TYPE__ I64;

typedef __UINT8_TYPE__  U8;
typedef __UINT16_TYPE__ U16;
typedef __UINT32_TYPE__ U32;
typedef __UINT64_TYPE__ U64;
#endif

#define U8_MAX  (U8)  0xFF
#define U16_MAX (U16) 0xFFFF
#define U32_MAX (U32) 0xFFFFFFFF
#define U64_MAX (U64) 0xFFFFFFFFFFFFFFFF

#define U8_MIN  (U8)  0
#define U16_MIN (U16) 0
#define U32_MIN (U32) 0
#define U64_MIN (U64) 0

#define I8_MAX  (I8)  (U8_MAX  >> 1)
#define I16_MAX (I16) (U16_MAX >> 1)
#define I32_MAX (I32) (U32_MAX >> 1)
#define I64_MAX (I64) (U64_MAX >> 1)

#define I8_MIN  (I8)  (~I8_MAX)
#define I16_MIN (I16) (~I16_MAX)
#define I32_MIN (I32) (~I32_MAX)
#define I64_MIN (I64) (~I64_MAX)

typedef float  F32;
typedef double F64;

typedef U8  B8;
typedef U16 B16;
typedef U32 B32;
typedef U64 B64;

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

/// Asset types
typedef U32 Asset_ID;