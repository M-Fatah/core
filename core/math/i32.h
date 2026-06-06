#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/math/u32.h"

inline static I32
i32_abs(I32 x)
{
	validate(x != I32_MIN, "[MATH][i32]: abs would overflow.");
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

inline static bool
i32_is_power_of_two(I32 x)
{
	return x > 0 && (x & (x - 1)) == 0;
}

inline static I32
i32_next_power_of_two(I32 x)
{
	validate(x >= 0, "[MATH][i32]: next_power_of_two input must be non-negative.");
	validate(x <= 0x40000000, "[MATH][i32]: next_power_of_two would overflow.");
	return (I32)u32_next_power_of_two((U32)x);
}

inline static I32
i32_previous_power_of_two(I32 x)
{
	validate(x > 0, "[MATH][i32]: previous_power_of_two input must be positive.");
	return (I32)u32_previous_power_of_two((U32)x);
}

inline static I32
i32_align_up(I32 x, I32 alignment)
{
	validate(x >= 0, "[MATH][i32]: align_up input must be non-negative.");
	validate(alignment > 0 && i32_is_power_of_two(alignment), "[MATH][i32]: align_up alignment must be a positive power of two.");
	U32 result = u32_align_up((U32)x, (U32)alignment);
	validate(result <= (U32)I32_MAX, "[MATH][i32]: align_up would overflow.");
	return (I32)result;
}

inline static I32
i32_align_down(I32 x, I32 alignment)
{
	validate(x >= 0, "[MATH][i32]: align_down input must be non-negative.");
	validate(alignment > 0 && i32_is_power_of_two(alignment), "[MATH][i32]: align_down alignment must be a positive power of two.");
	return (I32)u32_align_down((U32)x, (U32)alignment);
}

inline static I32
i32_rotate_left(I32 x, U32 shift)
{
	return (I32)u32_rotate_left((U32)x, shift);
}

inline static I32
i32_rotate_right(I32 x, U32 shift)
{
	return (I32)u32_rotate_right((U32)x, shift);
}

inline static U32
i32_popcount(I32 x)
{
	return u32_popcount((U32)x);
}

inline static U32
i32_leading_zero_count(I32 x)
{
	validate(x != 0, "[MATH][i32]: leading_zero_count input must be non-zero.");
	return u32_leading_zero_count((U32)x);
}

inline static U32
i32_trailing_zero_count(I32 x)
{
	validate(x != 0, "[MATH][i32]: trailing_zero_count input must be non-zero.");
	return u32_trailing_zero_count((U32)x);
}

inline static I32
i32_byte_swap(I32 x)
{
	return (I32)u32_byte_swap((U32)x);
}