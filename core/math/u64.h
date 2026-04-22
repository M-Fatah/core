#pragma once

#include <core/defines.h>

// ============================================================================
// U64 scalar helpers. U64_MAX lives in core/defines.h. No abs/sign for unsigned.
// ============================================================================

inline static U64
u64_min(U64 a, U64 b)
{
	return a < b ? a : b;
}

inline static U64
u64_max(U64 a, U64 b)
{
	return a > b ? a : b;
}

inline static U64
u64_clamp(U64 x, U64 lo, U64 hi)
{
	if (x < lo) return lo;
	if (x > hi) return hi;
	return x;
}
