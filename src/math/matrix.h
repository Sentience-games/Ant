#pragma once

#include "ant_types.h"

// TODO(soimn): Speed!

struct M2
{
    union
    {
        F32 e[4];
        F32 ed[2][2];
        
        struct
        {
            V2 i;
            V2 j;
        };
    };
};

struct M3
{
    union
    {
        F32 e[9];
        F32 ed[3][3];
        
        struct
        {
            V3 i;
            V3 j;
            V3 k;
        };
    };
};

struct M4
{
    union
    {
        F32 e[16];
        F32 ed[4][4];
        
        struct
        {
            V4 i;
            V4 j;
            V4 k;
            V4 w;
        };
    };
};

inline M2
Mat2(F32 e0, F32 e1, 
     F32 e2, F32 e3)
{
    return {e0, e1, e2, e3};
}

inline M3
Mat3(F32 e0, F32 e1, F32 e2, 
     F32 e3, F32 e4, F32 e5,
     F32 e6, F32 e7, F32 e8)
{
    return {e0, e1, e2, e3, e4, e5, e6, e7, e8};
}

inline M4
Mat4(F32 e0,  F32 e1,  F32 e2,  F32 e3,
     F32 e4,  F32 e5,  F32 e6,  F32 e7,
     F32 e8,  F32 e9,  F32 e10, F32 e11,
     F32 e12, F32 e13, F32 e14, F32 e15)
{
    return {e0, e1, e2, e3, e4, e5, e6, e7,
        e8, e9, e10, e11, e12, e13, e14, e15};
}

inline M2
M2Identity()
{
    return {1, 0,
        0, 1};
}

inline M3
M3Identity()
{
    return {1, 0, 0,
        0, 1, 0,
        0, 0, 1};
}

inline M4
M4Identity()
{
    return {1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1};
}

inline M2
Transpose(const M2& m)
{
    return {m.e[0], m.e[1],
        m.e[2], m.e[3]};
}

inline M3
Transpose(const M3& m)
{
    return {m.e[0], m.e[1], m.e[2],
        m.e[3], m.e[4], m.e[5],
        m.e[6], m.e[7], m.e[8]};
}

inline M4
Transpose(const M4& m)
{
    return {m.e[0], m.e[1], m.e[2], m.e[3],
        m.e[4], m.e[5], m.e[6], m.e[7],
        m.e[8], m.e[9], m.e[10], m.e[11],
        m.e[12], m.e[13], m.e[14], m.e[15]};
}

inline M2
operator * (const M2& m0, const M2& m1)
{
    M2 result = {};
    
    V2 row_0 = {m0.i.x, m0.j.x};
    V2 row_1 = {m0.i.y, m0.j.y};
    
    result.i.x = Inner(row_0, m1.i);
    result.i.y = Inner(row_1, m1.i);
    
    result.j.x = Inner(row_0, m1.j);
    result.j.y = Inner(row_1, m1.j);
    
    return result;
}

inline M3
operator * (const M3& m0, const M3& m1)
{
    M3 result = {};
    
    V3 row_0 = {m0.i.x, m0.j.x, m0.k.x};
    V3 row_1 = {m0.i.y, m0.j.y, m0.k.y};
    V3 row_2 = {m0.i.z, m0.j.z, m0.k.z};
    
    result.i.x = Inner(row_0, m1.i);
    result.i.y = Inner(row_1, m1.i);
    result.i.z = Inner(row_2, m1.i);
    
    result.j.x = Inner(row_0, m1.j);
    result.j.y = Inner(row_1, m1.j);
    result.j.z = Inner(row_2, m1.j);
    
    result.k.x = Inner(row_0, m1.k);
    result.k.y = Inner(row_1, m1.k);
    result.k.z = Inner(row_2, m1.k);
    
    return result;
}

inline M4
operator * (const M4& m0, const M4& m1)
{
    M4 result = {};
    
    V4 row_0 = {m0.i.x, m0.j.x, m0.k.x, m0.w.x};
    V4 row_1 = {m0.i.y, m0.j.y, m0.k.y, m0.w.y};
    V4 row_2 = {m0.i.z, m0.j.z, m0.k.z, m0.w.z};
    V4 row_3 = {m0.i.w, m0.j.w, m0.k.w, m0.w.w};
    
    result.i.x = Inner(row_0, m1.i);
    result.i.y = Inner(row_1, m1.i);
    result.i.z = Inner(row_2, m1.i);
    result.i.w = Inner(row_3, m1.i);
    
    result.j.x = Inner(row_0, m1.j);
    result.j.y = Inner(row_1, m1.j);
    result.j.z = Inner(row_2, m1.j);
    result.j.w = Inner(row_3, m1.j);
    
    result.k.x = Inner(row_0, m1.k);
    result.k.y = Inner(row_1, m1.k);
    result.k.z = Inner(row_2, m1.k);
    result.k.w = Inner(row_3, m1.k);
    
    result.w.x = Inner(row_0, m1.w);
    result.w.y = Inner(row_1, m1.w);
    result.w.z = Inner(row_2, m1.w);
    result.w.w = Inner(row_3, m1.w);
    
    return result;
}

inline V2
operator * (const M2& m, const V2& v)
{
    V2 result = {};
    
    V2 row_0 = {m.i.x, m.j.x};
    V2 row_1 = {m.i.y, m.j.y};
    
    result.x = Inner(row_0, v);
    result.y = Inner(row_1, v);
    
    return result;
}

inline V3
operator * (const M3& m, const V3& v)
{
    V3 result = {};
    
    V3 row_0 = {m.i.x, m.j.x, m.k.x};
    V3 row_1 = {m.i.y, m.j.y, m.k.y};
    V3 row_2 = {m.i.z, m.j.z, m.k.z};
    
    result.x = Inner(row_0, v);
    result.y = Inner(row_1, v);
    result.z = Inner(row_2, v);
    
    return result;
}

inline V4
operator * (const M4& m, const V4& v)
{
    V4 result = {};
    
    V4 row_0 = {m.i.x, m.j.x, m.k.x, m.w.x};
    V4 row_1 = {m.i.y, m.j.y, m.k.y, m.w.y};
    V4 row_2 = {m.i.z, m.j.z, m.k.z, m.w.z};
    V4 row_3 = {m.i.w, m.j.w, m.k.w, m.w.w};
    
    result.x = Inner(row_0, v);
    result.y = Inner(row_1, v);
    result.z = Inner(row_2, v);
    result.w = Inner(row_3, v);
    
    return result;
}