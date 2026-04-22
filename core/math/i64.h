#pragma once

#include <core/defines.h>

// ============================================================================
// I64 scalar helpers. I64_MIN / I64_MAX live in core/defines.h.
// ============================================================================

inline static I64
i64_abs(I64 x)
{
	return x < 0 ? -x : x;
}

inline static I64
i64_sign(I64 x)
{
	if (x > 0) return  1;
	if (x < 0) return -1;
	return 0;
}

inline static I64
i64_min(I64 a, I64 b)
{
	return a < b ? a : b;
}

inline static I64
i64_max(I64 a, I64 b)
{
	return a > b ? a : b;
}

inline static I64
i64_clamp(I64 x, I64 lo, I64 hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}
