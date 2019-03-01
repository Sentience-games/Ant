#pragma once

// TODO(soimn):
/*
 * - Validate existing functions
 * - Implement length, normalize, ...
 */

struct v2
{
	union
	{
		struct
		{
			float x, y;
		};

		float E[2];
	};
};

struct v3
{
	union
	{
		struct
		{
			float x, y, z;
		};

		struct
		{
			v2 xy;
			float _ignored01;
		};

		struct
		{
			v2 yz;
			float _ignored02;
		};

		float E[3];
	};
};

struct v4
{
	union
	{
		struct
		{
			float x, y, z, w;
		};

		struct
		{
			v2 xy, zw;
		};

		struct
		{
			float _ignored01;
			v2 yz;
			float _ignored02;
		};

		struct
		{
			v3 xyz;
			float _ignored03;
		};

		struct
		{
			float _ignored04;
			v3 yzw;
		};

		float E[4];
	};
};


/// V2

inline
v2 operator + (const v2& v_1, const v2& v_2)
{
	return {v_1.x + v_2.x, v_1.y + v_2.y};
}

inline
v2 operator - (const v2& v_1, const v2& v_2)
{
	return {v_1.x - v_2.x, v_1.y - v_2.y};
}

inline
v2 operator - (const v2& v)
{
	return {-v.x, -v.y};
}

inline
v2& operator += (v2& rhs, const v2& lhs)
{
	rhs.x += lhs.x;
	rhs.y += lhs.y;
	return rhs;
}

inline
v2& operator -= (v2& rhs, const v2& lhs)
{
	rhs += -lhs;
	return rhs;
}

inline
v2 operator * (float s, const v2& v)
{
	return {s * v.x, s * v.y};
}

inline
v2 operator * (const v2& v, float s)
{
	return s * v;
}

inline
v2& operator *= (v2& v, float s)
{
	v.x *= s;
	v.y *= s;
	return v;
}

inline
v2 operator / (float s, const v2& v)
{
	return {s / v.x, s / v.y};
}

inline
v2 operator / (const v2& v, float s)
{
	return s / v;
}

inline
v2& operator /= (v2& v, float s)
{
	v.x /= s;
	v.y /= s;
	return v;
}

inline float
Inner(const v2& v_1, const v2& v_2)
{
	return v_1.x * v_2.x + v_1.y * v_2.y;
}

inline v2
Hadamard(const v2& v_1, const v2& v_2)
{
	return {v_1.x * v_2.x, v_1.y * v_2.y};
}

inline float
LengthSq(const v2& v)
{
	return Inner(v, v);
}


/// V3

inline
v3 operator + (const v3& v_1, const v3& v_2)
{
	return {v_1.x + v_2.x, v_1.y + v_2.y, v_1.z + v_2.z};
};

inline
v3 operator - (const v3& v_1, const v3& v_2)
{
	return {v_1.x - v_2.x, v_1.y - v_2.y, v_1.z - v_2.z};
};

inline
v3 operator - (const v3& v)
{
	return {-v.x, -v.y, -v.z};
};

inline
v3& operator += (v3& rhs, const v3& lhs)
{
	rhs.x += lhs.x;
	rhs.y += lhs.y;
	rhs.z += lhs.z;
	return rhs;
};

inline
v3& operator -= (v3& rhs, const v3& lhs)
{
	rhs += -lhs;
	return rhs;
};

inline
v3 operator * (float s, const v3& v)
{
	return {s * v.x, s * v.y, s * v.z};
}

inline
v3 operator * (const v3& v, float s)
{
	return s * v;
}

inline
v3& operator *= (v3& v, float s)
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	return v;
}

inline
v3 operator / (float s, const v3& v)
{
	return {s / v.x, s / v.y, s / v.z};
}

inline
v3 operator / (const v3& v, float s)
{
	return s / v;
}

inline
v3& operator /= (v3& v, float s)
{
	v.x /= s;
	v.y /= s;
	v.z /= s;
	return v;
}

inline float
Inner(const v3& v_1, const v3& v_2)
{
	return v_1.x * v_2.x + v_1.y * v_2.y + v_1.z * v_2.z;
}

inline v3
Hadamard(const v3& v_1, const v3& v_2)
{
	return {v_1.x * v_2.x, v_1.y * v_2.y, v_1.z * v_2.z};
}

inline float
LengthSq(const v3& v)
{
	return Inner(v, v);
}


/// V4

inline
v4 operator + (const v4& v_1, const v4& v_2)
{
	return {v_1.x + v_2.x, v_1.y + v_2.y, v_1.z + v_2.z, v_1.w + v_2.w};
};

inline
v4 operator - (const v4& v_1, const v4& v_2)
{
	return {v_1.x - v_2.x, v_1.y - v_2.y, v_1.z - v_2.z, v_1.w - v_2.w};
};

inline
v4 operator - (const v4& v)
{
	return {-v.x, -v.y, -v.z, -v.w};
};

inline
v4& operator += (v4& rhs, const v4& lhs)
{
	rhs.x += lhs.x;
	rhs.y += lhs.y;
	rhs.z += lhs.z;
	rhs.w += lhs.w;
	return rhs;
};

inline
v4& operator -= (v4& rhs, const v4& lhs)
{
	rhs += -lhs;
	return rhs;
};

inline
v4 operator * (float s, const v4& v)
{
	return {s * v.x, s * v.y, s * v.z, s * v.w};
}

inline
v4 operator * (const v4& v, float s)
{
	return s * v;
}

inline
v4& operator *= (v4& v, float s)
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	v.w *= s;
	return v;
}

inline
v4 operator / (float s, const v4& v)
{
	return {s / v.x, s / v.y, s / v.z, s / v.w};
}

inline
v4 operator / (const v4& v, float s)
{
	return s / v;
}

inline
v4& operator /= (v4& v, float s)
{
	v.x /= s;
	v.y /= s;
	v.z /= s;
	v.w /= s;
	return v;
}

inline float
Inner(const v4& v_1, const v4& v_2)
{
	return v_1.x * v_2.x + v_1.y * v_2.y + v_1.z * v_2.z + v_1.w * v_2.w;
}

inline v4
Hadamard(const v4& v_1, const v4& v_2)
{
	return {v_1.x * v_2.x, v_1.y * v_2.y, v_1.z * v_2.z, v_1.w * v_2.w};
}

inline float
LengthSq(const v4& v)
{
	return Inner(v, v);
}
