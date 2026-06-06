#pragma once

#include "core/defines.h"
#include "core/validate.h"

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

inline static bool
u32_is_power_of_two(U32 x)
{
	return x != 0 && (x & (x - 1)) == 0;
}

inline static U32
u32_next_power_of_two(U32 x)
{
	validate(x <= 0x80000000u, "[MATH][u32]: next_power_of_two would overflow.");
	if (x <= 1)
		return 1;
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

inline static U32
u32_previous_power_of_two(U32 x)
{
	validate(x != 0, "[MATH][u32]: previous_power_of_two input must be non-zero.");
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x - (x >> 1);
}

inline static U32
u32_align_up(U32 x, U32 alignment)
{
	validate(u32_is_power_of_two(alignment), "[MATH][u32]: align_up alignment must be a power of two.");
	U32 mask = alignment - 1;
	validate(x <= U32_MAX - mask, "[MATH][u32]: align_up would overflow.");
	return (x + mask) & ~mask;
}

inline static U32
u32_align_down(U32 x, U32 alignment)
{
	validate(u32_is_power_of_two(alignment), "[MATH][u32]: align_down alignment must be a power of two.");
	return x & ~(alignment - 1);
}

inline static U32
u32_rotate_left(U32 x, U32 shift)
{
	shift &= 31;
	return (x << shift) | (x >> ((32 - shift) & 31));
}

inline static U32
u32_rotate_right(U32 x, U32 shift)
{
	shift &= 31;
	return (x >> shift) | (x << ((32 - shift) & 31));
}

inline static U32
u32_popcount(U32 x)
{
	U32 count = 0;
	while (x != 0)
	{
		x &= x - 1;
		++count;
	}
	return count;
}

inline static U32
u32_leading_zero_count(U32 x)
{
	validate(x != 0, "[MATH][u32]: leading_zero_count input must be non-zero.");
	U32 count = 0;
	for (U32 bit = 0x80000000u; (x & bit) == 0; bit >>= 1)
		++count;
	return count;
}

inline static U32
u32_trailing_zero_count(U32 x)
{
	validate(x != 0, "[MATH][u32]: trailing_zero_count input must be non-zero.");
	U32 count = 0;
	for (U32 bit = 1; (x & bit) == 0; bit <<= 1)
		++count;
	return count;
}

inline static U32
u32_byte_swap(U32 x)
{
	return ((x & 0x000000ffu) << 24)
		 | ((x & 0x0000ff00u) << 8)
		 | ((x & 0x00ff0000u) >> 8)
		 | ((x & 0xff000000u) >> 24);
}