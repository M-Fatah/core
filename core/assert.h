#pragma once

#ifdef DEBUG

#include "core/export.h"
#include "core/defines.h"

#if _MSC_VER
#define DEBUG_BREAK() __debugbreak()
#else
#define DEBUG_BREAK() __builtin_trap()
#endif

CORE_API void
_assert_report(const char *expression, const char *message, const char *file_name, i32 line_number);

#define ASSERT(expression, message)                                   \
	do {                                                              \
		if(!(expression)) {                                           \
			_assert_report(#expression, message, __FILE__, __LINE__); \
			DEBUG_BREAK();                                            \
		}                                                             \
	} while(0)

#else
#define ASSERT(expression, message)
#endif