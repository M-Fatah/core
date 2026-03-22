#pragma once

#include "core/defines.h"

inline static u32
next_power_of_two(u32 value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	++value;
	value += (value == 0);
	return value;
}

inline static u64
next_power_of_two(u64 value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value |= value >> 32;
	++value;
	value += (value == 0);
	return value;
}