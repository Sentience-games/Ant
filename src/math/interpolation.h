#pragma once

#include "ant_shared.h"
#include "math/vector.h"

inline F32
Lerp(F32 v_i, F32 t, F32 v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline V2
Lerp(const V2& v_i, F32 t, const V2& v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline V3
Lerp(const V3& v_i, F32 t, const V3& v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline V4
Lerp(const V4& v_i, F32 t, const V4& v_f)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * v_i + t * v_f;
}

inline V2
QuadBezier(F32 t, const V2& p_0, const V2& p_1, const V2& p_2)
{
	Assert(0.0f <= t && t <= 1.0f);
	return (1.0f - t) * (1.0f - t) * p_0 + 2 * (1.0f - t) * t * p_1 + t * t * p_2;
}

inline V2
CubeBezier(F32 t, const V2& p_0, const V2& p_1, const V2& p_2, const V2& p_3)
{
	Assert(0.0f <= t && t <= 1.0f);
    
	// (1.0f - t)^3 * p_0 + 3 * (1.0f - t)^2 * t * p_1 + 3 * (1.0f - t) * t^2 * p_2 + t^3 * p_3
	return (1.0f - t) * (1.0f - t) * (1.0f - t) * p_0
        + 3 * (1.0f - t) * (1.0f - t) * t * p_2
        + 3 * (1.0f - t) * t * t * p_3 
        + t * t * t * p_3;
}
