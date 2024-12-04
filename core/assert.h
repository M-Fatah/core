#pragma once

#include "core/export.h"

#include <source_location>

#undef assert

CORE_API void
assert(bool expression, const char *message, std::source_location source_location = std::source_location::current());

inline static void
assert(bool expression, std::source_location source_location = std::source_location::current())
{
	assert(expression, "", source_location);
}