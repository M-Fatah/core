#pragma once

#include "core/export.h"

#include <source_location>

CORE_API void
assert(bool expression, const char *message = "", std::source_location source_location = std::source_location::current());