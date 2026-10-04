#pragma once

#include "core/export.h"
#include "core/containers/slice.h"

struct Utf8_Character
{
	U32 code_point;
	U32 byte_count;
};

CORE_API Utf8_Character
utf8_decode(Slice<const char> text);

CORE_API I32
unicode_column_width(U32 code_point);