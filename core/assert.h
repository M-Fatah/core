#pragma once

#include "core/export.h"
#include "core/source_location.h"

CORE_API void
assert(bool expression, const char *message = "", Source_Location source_location = source_location_get());