#pragma once

#include "core/defines.h"
#include <source_location>

struct Source_Location
{
	const char *file_name;
	const char *function_name;
	u32 line_number;
	u32 column_number;
};

inline static consteval Source_Location
source_location_get(const char *file_name = __builtin_FILE(), const char *function_name = __builtin_FUNCTION(), u32 line_number = __builtin_LINE(), u32 column_number = __builtin_COLUMN())
{
	return Source_Location {
		.file_name = file_name,
		.function_name = function_name,
		.line_number = line_number,
		.column_number = column_number
	};
}