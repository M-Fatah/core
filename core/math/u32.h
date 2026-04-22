#pragma once

#include <core/defines.h>

// ============================================================================
// U32 scalar helpers. U32_MAX lives in core/defines.h. No abs/sign for unsigned.
// ============================================================================

inline static U32
u32_min(U32 a, U32 b)
{
	return a < b ? a : b;
}

inline static U32
u32_max(U32 a, U32 b)
{
	return a > b ? a : b;
}

inline static U32
u32_clamp(U32 x, U32 lo, U32 hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}
