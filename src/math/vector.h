#pragma once

#include "ant_types.h"

// TODO(soimn):
/*
 * - Validate existing functions
 * - Implement length, normalize, ...
 */

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
			V2 xy;
			F32 _ignored01;
		};
        
		struct
		{
			V2 yz;
			F32 _ignored02;
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
			V2 xy, zw;
		};
        
		struct
		{
			F32 _ignored01;
			V2 yz;
			F32 _ignored02;
		};
        
		struct
		{
			V3 xyz;
			F32 _ignored03;
		};
        
		struct
		{
			F32 _ignored04;
			V3 yzw;
		};
        
		F32 e[4];
	};
};


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
Hadamard(const V3& v_1, const V3& v_2)
{
	return {v_1.x * v_2.x, v_1.y * v_2.y, v_1.z * v_2.z};
}

inline F32
LengthSq(const V3& v)
{
	return Inner(v, v);
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

inline V4
& operator /= (V4& v, F32 s)
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
