#pragma once

#include "ant_types.h"

#include "math/vector.h"

struct M2
{
    union
    {
        F32 e[2][2];
        F32 el[4];
        
        struct
        {
            V2 row_0;
            V2 row_1;
        };
    };
};

struct M3
{
    union
    {
        F32 e[3][3];
        F32 el[9];
        
        struct
        {
            V3 row_0;
            V3 row_1;
            V3 row_2;
        };
        
    };
};

struct M4
{
    union
    {
        F32 e[4][4];
        F32 el[16];
        
        struct
        {
            V4 row_0;
            V4 row_1;
            V4 row_2;
            V4 row_3;
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