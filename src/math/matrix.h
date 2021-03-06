#pragma once

#include "ant_shared.h"

#include "math/vector.h"
#include "math/trigonometry.h"

union M2
{
    F32 e[4];
    F32 ed[2][2];
    
    struct
    {
        V2 i;
        V2 j;
    };
};

union M3
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

union M4
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

struct M2_Inv
{
    M2 m;
    M2 inv;
};

struct M3_Inv
{
    M3 m;
    M3 inv;
};

struct M4_Inv
{
    M4 m;
    M4 inv;
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

inline M2
RowsM2(const V2& r0, const V2& r1)
{
    return {r0.x, r1.x,
        r0.y, r1.y};
}

inline M3
RowsM3(const V3& r0, const V3& r1, const V3& r2)
{
    return {r0.x, r1.x, r2.x,
        r0.y, r1.y, r2.y,
        r0.z, r1.z, r2.z};
}

inline M4
RowsM4(const V4& r0, const V4& r1, const V4& r2, const V4& r3)
{
    return {r0.x, r1.x, r2.x, r3.x,
        r0.y, r1.y, r2.y, r3.y,
        r0.z, r1.z, r2.z, r3.z,
        r0.w, r1.w, r2.w, r3.w};
}

inline M2
ColumnsM2(const V2& i, const V2& j)
{
    return {i.x, i.y,
        j.x, j.y};
}

inline M3
ColumnsM3(const V3& i, const V3& j, const V3& k)
{
    return {i.x, i.y, i.z,
        j.x, j.y, j.z,
        k.x, k.y, k.z};
}

inline M4
ColumnsM4(const V4& i, const V4& j, const V4& k, const V4& w)
{
    return {i.x, i.y, i.z, i.w,
        j.x, j.y, j.z, j.w,
        k.x, k.y, k.z, k.w,
        w.x, w.y, w.z, w.w};
}

inline M4
Rotation(Quat q)
{
    M4 result = {};
    
    Quat q_n = Normalized(q);
    
    result.i.xyz = Rotate({1, 0, 0}, q_n);
    result.j.xyz = Rotate({0, 1, 0}, q_n);
    result.k.xyz = Rotate({0, 0, 1}, q_n);
    result.w     = {0, 0, 0, 1};
    
    return result;
}

inline M4
Translation(const V3& delta)
{
    return {1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        delta.x, delta.y, delta.z, 1};
}

inline M4
Scale(const V3& delta)
{
    return {delta.x, 0, 0, 0,
        0, delta.y, 0, 0,
        0, 0, delta.z, 0,
        0, 0, 0, 1};
}

inline M2
Transpose(const M2& m)
{
    M2 result = m;
    
    result.i = {m.i.x, m.j.x};
    result.j = {m.i.y, m.j.y};
    
    return result;
}

inline M3
Transpose(const M3& m)
{
    M3 result = m;
    
    result.i = {m.i.x, m.j.x, m.k.x};
    result.j = {m.i.y, m.j.y, m.k.y};
    result.k = {m.i.z, m.j.z, m.k.z};
    
    return result;
}

inline M4
Transpose(const M4& m)
{
    M4 result = m;
    
    result.i = {m.i.x, m.j.x, m.k.x, m.w.x};
    result.j = {m.i.y, m.j.y, m.k.y, m.w.y};
    result.k = {m.i.z, m.j.z, m.k.z, m.w.z};
    result.w = {m.i.w, m.j.w, m.k.w, m.w.w};
    
    return result;
}

inline M4_Inv
ModelMatrix(Transform transform)
{
    M4_Inv result = {};
    
    Assert(transform.rotation.x || transform.rotation.y || transform.rotation.z || transform.rotation.w);
    
    M4 rotation_matrix = Rotation(transform.rotation);
    
    result.m   = Translation(transform.position) * rotation_matrix * Scale(transform.scale);
    result.inv = Translation(-transform.position) * Transpose(rotation_matrix) * Scale(1 / transform.scale);
    
    return result;
}

// TODO(soimn): Verify this
inline M4_Inv
ViewMatrix(V3 position, Quat rotation)
{
    M4_Inv result = {};
    
    Assert(rotation.x || rotation.y || rotation.z || rotation.w);
    
    M4 rotation_matrix = Rotation(rotation);
    
    result.m   = Translation(-position) * Transpose(rotation_matrix);
    result.inv = Translation(position) * rotation_matrix;
    
    return result;
}

// NOTE(soimn): This expects z-out, y-up
inline M4_Inv
PerspectiveMatrix(F32 aspect_ratio, F32 fov, F32 near, F32 far)
{
    M4_Inv result = {};
    
    F32 tan_fov = Tan(fov / 2.0f);
    
    // x, y remapping to -1 - 1
    result.m.i.x = 1.0f / tan_fov;
    result.m.j.y = aspect_ratio / tan_fov;
    
    // Z remapping to 0 - 1
    result.m.k.z = far / (near - far);
    result.m.w.z = (near * far) / (near - far);
    
    result.m.k.w = -1.0f;
    
    // NOTE(soimn): Inverse - computed by Wolfram|Alpha
    result.inv.i.x = tan_fov;
    result.inv.j.y = tan_fov / aspect_ratio;
    
    result.inv.k.w = 1.0f / far - 1.0f / near;
    result.inv.w.z = -1.0f;
    result.inv.w.w = 1.0f / near;
    
    return result;
}

// TODO(soimn): Consider changing fov to width
inline M4_Inv
OrthographicMatrix(F32 aspect_ratio, F32 fov, F32 near, F32 far)
{
    M4_Inv result = {};
    
    F32 tan_fov = Tan(fov / 2.0f) * far;
    
    // x, y remapping to -1 - 1
    result.m.i.x = 1.0f / tan_fov;
    result.m.j.y = aspect_ratio / tan_fov;
    
    result.m.k.z = 1.0f / (near - far);
    result.m.w.z = near / (near - far);
    result.m.w.w = 1.0f;
    
    // NOTE(soimn): Inverse - computed by Wolfram|Alpha
    result.inv.i.x = tan_fov;
    result.inv.j.y = tan_fov / aspect_ratio;
    
    result.inv.k.z = near - far;
    result.inv.w.z = -near;
    result.inv.w.w = 1.0f;
    
    return result;
}