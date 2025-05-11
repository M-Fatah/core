#pragma once

#include "core/defines.h"

// TODO: Add variant for u32, u64, ..etc
inline static u64
next_power_of_two(i32 value)
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