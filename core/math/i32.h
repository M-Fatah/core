#pragma once

#include <core/defines.h>

// ============================================================================
// I32 scalar helpers. I32_MIN / I32_MAX live in core/defines.h.
// ============================================================================

inline static I32
i32_abs(I32 x)
{
	return x < 0 ? -x : x;
}

inline static I32
i32_sign(I32 x)
{
	if (x > 0) return  1;
	if (x < 0) return -1;
	return 0;
}

inline static I32
i32_min(I32 a, I32 b)
{
	return a < b ? a : b;
}

inline static I32
i32_max(I32 a, I32 b)
{
	return a > b ? a : b;
}

inline static I32
i32_clamp(I32 x, I32 lo, I32 hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}
