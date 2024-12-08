#include "core/validate.h"

#include "core/defines.h"
#include "core/logger.h"

void
validate(bool expression, const char *message, Source_Location source_location)
{
	unused(expression, message, source_location);
	#if DEBUG
		if (expression)
			return;

		log_fatal("Validation failure with message: '{}', file: '{}', line: '{}'.", message, source_location.file_name, source_location.line_number);

		#if COMPILER_MSVC
			__debugbreak();
		#else
			__builtin_trap();
		#endif
	#endif
}