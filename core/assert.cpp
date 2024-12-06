#include "core/assert.h"

#include "core/defines.h"
#include "core/logger.h"

void
assert(bool expression, const char *message, Source_Location source_location)
{
	#if DEBUG
		if (expression)
			return;

		LOG_FATAL("Assertion failure: '{}', message: '{}', file: '{}', line: '{}'.", expression, message, source_location.file_name, source_location.line_number);

		// TODO: Print stack.

		#if _MSC_VER // TODO: Add compiler definition.
			__debugbreak();
		#else
			__builtin_trap();
		#endif
	#else
		unused(expression, message, source_location);
	#endif
}