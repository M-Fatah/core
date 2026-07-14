#pragma once

#include "core/defines.h"
#include "core/export.h"
#include "core/containers/array.h"
#include "core/containers/slice.h"
#include "core/memory/allocator.h"

struct Command_Line_Option_Desc
{
	const char *name;
	char short_name;
	bool requires_value;
};

enum Command_Line_Error_Code
{
	COMMAND_LINE_ERROR_UNKNOWN_OPTION,
	COMMAND_LINE_ERROR_MISSING_VALUE,
	COMMAND_LINE_ERROR_UNEXPECTED_VALUE
};

struct Command_Line_Error
{
	Command_Line_Error_Code code;
	const char *argument;
};

struct Command_Line_Option
{
	const char *name;
	Array<const char *> values;
};

struct Command_Line
{
	const char *executable;
	Array<Command_Line_Option> options;
	Array<const char *> positionals;
	Array<Command_Line_Error> errors;
};

CORE_API Command_Line
command_line_init(char **arguments, I32 argument_count, Slice<const Command_Line_Option_Desc> option_descs, memory::Allocator *allocator = memory::heap_allocator());

CORE_API void
command_line_deinit(Command_Line &self);

CORE_API bool
command_line_has_errors(const Command_Line &self);

CORE_API bool
command_line_has_option(const Command_Line &self, const char *option_name);

CORE_API Slice<const char *>
command_line_get_option_values(const Command_Line &self, const char *option_name);