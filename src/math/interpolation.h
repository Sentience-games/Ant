#pragma once

#include "ant_types.h"
#include "math/vector.h"

inline f32
Lerp(f32 v_i, f32 t, f32 v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline v2
Lerp(const v2& v_i, f32 t, const v2& v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline v3
Lerp(const v3& v_i, f32 t, const v3& v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline v4
Lerp(const v4& v_i, f32 t, const v4& v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline v2
QuadBezier(f32 t, const v2& p_0, const v2& p_1, const v2& p_2)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * (1.0f - t) * p_0 + 2 * (1.0f - t) * t * p_1 + t * t * p_2;
}

inline v2
CubeBezier(f32 t, const v2& p_0, const v2& p_1, const v2& p_2, const v2& p_3)
{
	Assert(0.0f <= t && t <= 1.0f);

	// (1.0f - t)^3 * p_0 + 3 * (1.0f - t)^2 * t * p_1 + 3 * (1.0f - t) * t^2 * p_2 + t^3 * p_3
	return (1.0f - t) * (1.0f - t) * (1.0f - t) * p_0
		   + 3 * (1.0f - t) * (1.0f - t) * t * p_2
		   + 3 * (1.0f - t) * t * t * p_3 
		   + t * t * t * p_3;
}
