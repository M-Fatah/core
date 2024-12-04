#include "core/assert.h"

#include "core/defines.h"
#include "core/logger.h"

void
assert(bool expression, const char *message, std::source_location source_location)
{
	#if DEBUG
		if (expression)
			return;

		LOG_FATAL("Assertion failure: '{}', message: '{}', file: '{}', line: '{}'.", expression, message, source_location.file_name(), source_location.line());

		// TODO: Print stack.

		#if _MSC_VER // TODO: Add compiler definition.
			#define DEBUG_BREAK() __debugbreak()
		#else
			#define DEBUG_BREAK() __builtin_trap()
		#endif
	#else
		unused(expression, message, source_location);
	#endif
}