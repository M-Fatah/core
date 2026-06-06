#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/u64.h"

inline static I64
i64_abs(I64 x)
{
	validate(x != I64_MIN, "[MATH][i64]: abs would overflow.");
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

inline static bool
i64_is_power_of_two(I64 x)
{
	return x > 0 && (x & (x - 1)) == 0;
}

inline static I64
i64_next_power_of_two(I64 x)
{
	validate(x >= 0, "[MATH][i64]: next_power_of_two input must be non-negative.");
	validate(x <= 0x4000000000000000ll, "[MATH][i64]: next_power_of_two would overflow.");
	return (I64)u64_next_power_of_two((U64)x);
}

inline static I64
i64_previous_power_of_two(I64 x)
{
	validate(x > 0, "[MATH][i64]: previous_power_of_two input must be positive.");
	return (I64)u64_previous_power_of_two((U64)x);
}

inline static I64
i64_align_up(I64 x, I64 alignment)
{
	validate(x >= 0, "[MATH][i64]: align_up input must be non-negative.");
	validate(alignment > 0 && i64_is_power_of_two(alignment), "[MATH][i64]: align_up alignment must be a positive power of two.");
	U64 result = u64_align_up((U64)x, (U64)alignment);
	validate(result <= (U64)I64_MAX, "[MATH][i64]: align_up would overflow.");
	return (I64)result;
}

inline static I64
i64_align_down(I64 x, I64 alignment)
{
	validate(x >= 0, "[MATH][i64]: align_down input must be non-negative.");
	validate(alignment > 0 && i64_is_power_of_two(alignment), "[MATH][i64]: align_down alignment must be a positive power of two.");
	return (I64)u64_align_down((U64)x, (U64)alignment);
}

inline static I64
i64_rotate_left(I64 x, U32 shift)
{
	return (I64)u64_rotate_left((U64)x, shift);
}

inline static I64
i64_rotate_right(I64 x, U32 shift)
{
	return (I64)u64_rotate_right((U64)x, shift);
}

inline static U32
i64_popcount(I64 x)
{
	return u64_popcount((U64)x);
}

inline static U32
i64_leading_zero_count(I64 x)
{
	validate(x != 0, "[MATH][i64]: leading_zero_count input must be non-zero.");
	return u64_leading_zero_count((U64)x);
}

inline static U32
i64_trailing_zero_count(I64 x)
{
	validate(x != 0, "[MATH][i64]: trailing_zero_count input must be non-zero.");
	return u64_trailing_zero_count((U64)x);
}

inline static I64
i64_byte_swap(I64 x)
{
	return (I64)u64_byte_swap((U64)x);
}