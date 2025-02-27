#include "core/validate.h"

#include "core/defines.h"
#include "core/log.h"

void
validate(bool expression, const char *message, Source_Location source_location)
{
	unused(expression, message, source_location);
	#if DEBUG
		if (expression)
			return;

		log_to_console(LOG_TAG_FATAL, "[{}:{}]: Validation failure with message '{}'.", source_location.file_name, source_location.line_number, message);

		#if COMPILER_MSVC
			__debugbreak();
		#else
			__builtin_trap();
		#endif
	#endif
}