#include "core/assert.h"

#include "core/logger.h"

void
_assert_report(const char *expression, const char *message, const char *file_name, i32 line_number)
{
	LOG_FATAL("Assertion failure: '{}', message: '{}', file: '{}', line: '{}'.", expression, message, file_name, line_number);
}