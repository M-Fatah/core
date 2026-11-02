#pragma once

#include "core/export.h"
#include "core/containers/view.h"

struct Unicode_Utf8_Character
{
	U32 code_point;
	U32 byte_count;
};

CORE_API Unicode_Utf8_Character
unicode_decode_utf8(String_View text);

CORE_API I32
unicode_get_column_width(U32 code_point);