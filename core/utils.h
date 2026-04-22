#pragma once

#include "core/defines.h"

// TODO: Add variant for U32, U64, ..etc
inline static U64
next_power_of_two(I32 value)
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