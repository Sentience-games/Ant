#pragma once

#include "ant_types.h"

#include "math/exponentiation.h"

struct V2
{
	union
	{
		struct
		{
			F32 x, y;
		};
        
		F32 e[2];
	};
};

struct V3
{
	union
	{
		struct
		{
			F32 x, y, z;
		};
        
        struct
        {
            F32 r, g, b;
        };
        
		struct
		{
			V2 xy;
			F32 _ignored0;
		};
        
		struct
		{
			V2 yz;
			F32 _ignored1;
		};
        
        struct
        {
            V2 rg;
            F32 _ignored2;
        };
        
        struct
        {
            F32 _ignored3;
            V2 gb;
        };
        
		F32 e[3];
	};
};

struct V4
{
	union
	{
		struct
		{
			F32 x, y, z, w;
		};
        
        struct
        {
            F32 r, g, b, a;
        };
        
		struct
		{
			V2 xy, zw;
		};
        
        struct
        {
            F32 rg, ba;
        };
        
		struct
		{
			F32 _ignored0;
			V2 yz;
			F32 _ignored1;
		};
        
        struct
        {
            F32 _ignored2;
            V2 gb;
            F32 _ignored3;
        };
        
		struct
		{
			V3 xyz;
			F32 _ignored4;
		};
        
		struct
		{
			F32 _ignored5;
			V3 yzw;
		};
        
        struct
        {
            V3 rgb;
            F32 _ignored6;
        };
        
        struct
        {
            F32 _ignored7;
            V3 bga;
        };
        
		F32 e[4];
	};
};

typedef V4 Quat;

/// V2

inline V2
Vec2(F32 x, F32 y)
{
	return {x, y};
}

inline bool
operator == (const V2& v_1, const V2& v_2)
{
	return (v_1.x == v_2.x && v_1.y == v_2.y);
}

inline V2
operator + (const V2& v_1, const V2& v_2)
{
	return {v_1.x + v_2.x, v_1.y + v_2.y};
}

inline V2
operator - (const V2& v_1, const V2& v_2)
{
	return {v_1.x - v_2.x, v_1.y - v_2.y};
}

inline V2
operator - (const V2& v)
{
	return {-v.x, -v.y};
}

inline V2&
operator += (V2& rhs, const V2& lhs)
{
	rhs.x += lhs.x;
	rhs.y += lhs.y;
	return rhs;
}

inline V2&
operator -= (V2& rhs, const V2& lhs)
{
	rhs += -lhs;
	return rhs;
}

inline V2
operator * (F32 s, const V2& v)
{
	return {s * v.x, s * v.y};
}

inline V2
operator * (const V2& v, F32 s)
{
	return s * v;
}

inline V2&
operator *= (V2& v, F32 s)
{
	v.x *= s;
	v.y *= s;
	return v;
}

inline V2
operator / (F32 s, const V2& v)
{
	return {s / v.x, s / v.y};
}

inline V2
operator / (const V2& v, F32 s)
{
	return v * (1.0f / s);
}

inline V2&
operator /= (V2& v, F32 s)
{
	v.x /= s;
	v.y /= s;
	return v;
}

inline F32
Inner(const V2& v_1, const V2& v_2)
{
	return v_1.x * v_2.x + v_1.y * v_2.y;
}

inline V2
Hadamard(const V2& v_1, const V2& v_2)
{
	return {v_1.x * v_2.x, v_1.y * v_2.y};
}

inline F32
LengthSq(const V2& v)
{
	return Inner(v, v);
}

inline F32
Length(const V2& v)
{
    return Sqrt(Inner(v, v));
}

inline V2
Normalize(const V2& v)
{
    return v / Length(v);
}


/// V3

inline V3
Vec3(F32 x, F32 y, F32 z)
{
	return {x, y, z};
}

inline V3
operator + (const V3& v_1, const V3& v_2)
{
	return {v_1.x + v_2.x, v_1.y + v_2.y, v_1.z + v_2.z};
};

inline V3
operator - (const V3& v_1, const V3& v_2)
{
	return {v_1.x - v_2.x, v_1.y - v_2.y, v_1.z - v_2.z};
};

inline V3
operator - (const V3& v)
{
	return {-v.x, -v.y, -v.z};
};

inline V3&
operator += (V3& rhs, const V3& lhs)
{
	rhs.x += lhs.x;
	rhs.y += lhs.y;
	rhs.z += lhs.z;
	return rhs;
};

inline V3&
operator -= (V3& rhs, const V3& lhs)
{
	rhs += -lhs;
	return rhs;
};

inline V3
operator * (F32 s, const V3& v)
{
	return {s * v.x, s * v.y, s * v.z};
}

inline V3
operator * (const V3& v, F32 s)
{
	return s * v;
}

inline V3&
operator *= (V3& v, F32 s)
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	return v;
}

inline V3
operator / (F32 s, const V3& v)
{
	return {s / v.x, s / v.y, s / v.z};
}

inline V3
operator / (const V3& v, F32 s)
{
	return v * (1.0f / s);
}

inline V3&
operator /= (V3& v, F32 s)
{
	v.x /= s;
	v.y /= s;
	v.z /= s;
	return v;
}

inline F32
Inner(const V3& v_1, const V3& v_2)
{
	return v_1.x * v_2.x + v_1.y * v_2.y + v_1.z * v_2.z;
}

inline V3
Cross(const V3& v_1, const V3& v_2)
{
    return {v_1.y * v_2.z - v_1.z * v_2.y,
        v_1.z * v_2.x - v_1.x * v_2.z,
        v_1.x * v_2.y - v_1.y * v_2.x};
}

inline V3
Hadamard(const V3& v_1, const V3& v_2)
{
	return {v_1.x * v_2.x, v_1.y * v_2.y, v_1.z * v_2.z};
}

inline F32
LengthSq(const V3& v)
{
	return Inner(v, v);
}

inline F32
Length(const V3& v)
{
    return Sqrt(Inner(v, v));
}

inline V3
Normalize(const V3& v)
{
    return v / Length(v);
}

/// V4

inline V4
Vec4(F32 x, F32 y, F32 z, F32 w)
{
	return {x, y, z, w};
}

inline V4
operator + (const V4& v_1, const V4& v_2)
{
	return {v_1.x + v_2.x, v_1.y + v_2.y, v_1.z + v_2.z, v_1.w + v_2.w};
};

inline V4
operator - (const V4& v_1, const V4& v_2)
{
	return {v_1.x - v_2.x, v_1.y - v_2.y, v_1.z - v_2.z, v_1.w - v_2.w};
};

inline V4
operator - (const V4& v)
{
	return {-v.x, -v.y, -v.z, -v.w};
};

inline V4&
operator += (V4& rhs, const V4& lhs)
{
	rhs.x += lhs.x;
	rhs.y += lhs.y;
	rhs.z += lhs.z;
	rhs.w += lhs.w;
	return rhs;
};

inline V4&
operator -= (V4& rhs, const V4& lhs)
{
	rhs += -lhs;
	return rhs;
};

inline V4
operator * (F32 s, const V4& v)
{
	return {s * v.x, s * v.y, s * v.z, s * v.w};
}

inline V4
operator * (const V4& v, F32 s)
{
	return s * v;
}

inline V4&
operator *= (V4& v, F32 s)
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	v.w *= s;
	return v;
}

inline V4
operator / (F32 s, const V4& v)
{
	return {s / v.x, s / v.y, s / v.z, s / v.w};
}

inline V4
operator / (const V4& v, F32 s)
{
	return v * (1.0f / s);
}

inline V4&
operator /= (V4& v, F32 s)
{
	v.x /= s;
	v.y /= s;
	v.z /= s;
	v.w /= s;
	return v;
}

inline F32
Inner(const V4& v_1, const V4& v_2)
{
	return v_1.x * v_2.x + v_1.y * v_2.y + v_1.z * v_2.z + v_1.w * v_2.w;
}

inline V4
Hadamard(const V4& v_1, const V4& v_2)
{
	return {v_1.x * v_2.x, v_1.y * v_2.y, v_1.z * v_2.z, v_1.w * v_2.w};
}

inline F32
LengthSq(const V4& v)
{
	return Inner(v, v);
}

inline F32
Length(const V4& v)
{
    return Sqrt(Inner(v, v));
}

inline V4
Normalize(const V4& v)
{
    return v / Length(v);
}

// Quaternion

// TODO(soimn): Sin, cosine

inline V4
operator * (const Quat& q_1, const Quat& q_2)
{
    F32 real  = q_1.w * q_2.w - Inner(q_1.xyz, q_2.xyz);
    V3 vector = q_1.w * q_2.xyz + q_2.w * q_1.xyz + Cross(q_1.xyz, q_2.xyz);
    
    return {vector.x, vector.y, vector.z, real};
}

inline Quat
Conjugate(const Quat& q)
{
    return {-q.x, -q.y, -q.z, q.w};
}

inline V3
Rotate(V3 v, Quat q)
{
    return ((q * Vec4(v.x, v.y, v.z, 0.0f)) * Conjugate(q)).xyz;
}