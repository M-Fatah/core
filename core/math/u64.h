#pragma once

#include "core/defines.h"
#include "core/validate.h"

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

inline static bool
u64_is_power_of_two(U64 x)
{
	return x != 0 && (x & (x - 1)) == 0;
}

inline static U64
u64_next_power_of_two(U64 x)
{
	validate(x <= 0x8000000000000000ull, "[MATH][U64]: next_power_of_two would overflow.");
	if (x <= 1)
		return 1;
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	return x + 1;
}

inline static U64
u64_previous_power_of_two(U64 x)
{
	validate(x != 0, "[MATH][U64]: previous_power_of_two input must be non-zero.");
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	return x - (x >> 1);
}

inline static U64
u64_align_up(U64 x, U64 alignment)
{
	validate(u64_is_power_of_two(alignment), "[MATH][U64]: align_up alignment must be a power of two.");
	U64 mask = alignment - 1;
	validate(x <= U64_MAX - mask, "[MATH][U64]: align_up would overflow.");
	return (x + mask) & ~mask;
}

inline static U64
u64_align_down(U64 x, U64 alignment)
{
	validate(u64_is_power_of_two(alignment), "[MATH][U64]: align_down alignment must be a power of two.");
	return x & ~(alignment - 1);
}

inline static U64
u64_rotate_left(U64 x, U32 shift)
{
	shift &= 63;
	return (x << shift) | (x >> ((64 - shift) & 63));
}

inline static U64
u64_rotate_right(U64 x, U32 shift)
{
	shift &= 63;
	return (x >> shift) | (x << ((64 - shift) & 63));
}

inline static U32
u64_popcount(U64 x)
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
u64_leading_zero_count(U64 x)
{
	validate(x != 0, "[MATH][U64]: leading_zero_count input must be non-zero.");
	U32 count = 0;
	for (U64 bit = 0x8000000000000000ull; (x & bit) == 0; bit >>= 1)
		++count;
	return count;
}

inline static U32
u64_trailing_zero_count(U64 x)
{
	validate(x != 0, "[MATH][U64]: trailing_zero_count input must be non-zero.");
	U32 count = 0;
	for (U64 bit = 1; (x & bit) == 0; bit <<= 1)
		++count;
	return count;
}

inline static U64
u64_byte_swap(U64 x)
{
	return ((x & 0x00000000000000ffull) << 56)
		 | ((x & 0x000000000000ff00ull) << 40)
		 | ((x & 0x0000000000ff0000ull) << 24)
		 | ((x & 0x00000000ff000000ull) << 8)
		 | ((x & 0x000000ff00000000ull) >> 8)
		 | ((x & 0x0000ff0000000000ull) >> 24)
		 | ((x & 0x00ff000000000000ull) >> 40)
		 | ((x & 0xff00000000000000ull) >> 56);
}