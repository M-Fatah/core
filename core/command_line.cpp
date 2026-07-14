#include "core/command_line.h"

#include "core/validate.h"

inline static void
_command_line_parse(char **arguments, I32 argument_count, Slice<const Command_Line_Option_Desc> option_descs, Array<Command_Line_Option> &options, Array<const char *> &positionals, Array<Command_Line_Error> &errors)
{
	if (argument_count == 0)
		return;

	struct Parsed_Option
	{
		const Command_Line_Option_Desc *option_desc;
		const char *value;
	};

	constexpr auto parse_option = [](Slice<const Command_Line_Option_Desc> option_descs, Slice<const char> argument) -> Parsed_Option {
		if (argument.data[1] == '-')
		{
			const char *option_name_end = argument.data + 2;
			while (*option_name_end != '\0' && *option_name_end != '=')
				++option_name_end;

			Slice<const char> option_name = slice_from(argument.data + 2, option_name_end);
			const char *value = *option_name_end == '=' ? option_name_end + 1 : nullptr;
			for (const Command_Line_Option_Desc &option_desc : option_descs)
				if (option_name == option_desc.name)
					return Parsed_Option{.option_desc = &option_desc, .value = value};
			return Parsed_Option{};
		}
		else
		{
			const char *value = argument.count > 2 ? argument.data + 2 : nullptr;
			for (const Command_Line_Option_Desc &option_desc : option_descs)
				if (option_desc.short_name == argument.data[1])
					return Parsed_Option{.option_desc = &option_desc, .value = value};
			return Parsed_Option{};
		}
	};

	constexpr auto push_option = [](Array<Command_Line_Option> &options, const char *option_name, const char *value) -> void {
		Slice<const char> option_name_slice = slice_from(option_name);
		for (Command_Line_Option &option : options)
		{
			if (option_name_slice == option.name)
			{
				array_push(option.values, value);
				return;
			}
		}

		array_push(options, Command_Line_Option{.name = option_name, .values = array_init<const char *>(options.allocator)});
		array_push(array_back(options).values, value);
	};

	for (U64 i = 0; i < option_descs.count; ++i)
	{
		const Command_Line_Option_Desc &option_desc = option_descs[i];
		Slice<const char> name = slice_from(option_desc.name);
		validate(name.count > 0, "[COMMAND_LINE]: Option description name is invalid.");
		validate(name[0] != '-', "[COMMAND_LINE]: Option description name cannot start with '-'.");
		validate(option_desc.short_name != '-', "[COMMAND_LINE]: Option description short name cannot be '-'.");
		for (char character : name)
			validate(character != '=', "[COMMAND_LINE]: Option description name cannot contain '='.");

		for (U64 j = 0; j < i; ++j)
		{
			const Command_Line_Option_Desc &previous_option_desc = option_descs[j];
			validate(name != previous_option_desc.name, "[COMMAND_LINE]: Option description name is duplicated.");
			validate(option_desc.short_name == '\0' || option_desc.short_name != previous_option_desc.short_name, "[COMMAND_LINE]: Option description short name is duplicated.");
		}
	}

	bool options_enabled = true;
	for (I32 i = 1; i < argument_count; ++i)
	{
		const char *argument = arguments[i];
		Slice<const char> argument_slice = slice_from(argument);

		if (options_enabled && argument_slice == "--")
		{
			options_enabled = false;
			continue;
		}

		if (options_enabled == false || argument_slice.count < 2 || argument_slice[0] != '-')
		{
			array_push(positionals, argument);
			continue;
		}

		auto [option_desc, value] = parse_option(option_descs, argument_slice);
		if (option_desc == nullptr)
		{
			array_push(errors, Command_Line_Error{.code = COMMAND_LINE_ERROR_UNKNOWN_OPTION, .argument = argument});
			continue;
		}

		if (option_desc->requires_value)
		{
			if (value == nullptr && i + 1 < argument_count)
				value = arguments[++i];

			if (value == nullptr || value[0] == '\0')
			{
				array_push(errors, Command_Line_Error{.code = COMMAND_LINE_ERROR_MISSING_VALUE, .argument = argument});
				continue;
			}

			push_option(options, option_desc->name, value);
		}
		else
		{
			if (value != nullptr)
				array_push(errors, Command_Line_Error{.code = COMMAND_LINE_ERROR_UNEXPECTED_VALUE, .argument = argument});
			else
				push_option(options, option_desc->name, nullptr);
		}
	}
}

Command_Line
command_line_init(char **arguments, I32 argument_count, Slice<const Command_Line_Option_Desc> option_descs, memory::Allocator *allocator)
{
	Array<Command_Line_Option> options = array_init<Command_Line_Option>(allocator);
	Array<const char *> positionals    = array_init<const char *>(allocator);
	Array<Command_Line_Error> errors   = array_init<Command_Line_Error>(allocator);
	_command_line_parse(arguments, argument_count, option_descs, options, positionals, errors);
	return Command_Line {
		.executable = argument_count > 0 ? arguments[0] : nullptr,
		.options = options,
		.positionals = positionals,
		.errors = errors
	};
}

void
command_line_deinit(Command_Line &self)
{
	array_deinit(self.errors);
	array_deinit(self.positionals);
	for (Command_Line_Option &option : self.options)
		array_deinit(option.values);
	array_deinit(self.options);
	self = Command_Line{};
}

bool
command_line_has_errors(const Command_Line &self)
{
	return self.errors.count > 0;
}

bool
command_line_has_option(const Command_Line &self, const char *option_name)
{
	return command_line_get_option_values(self, option_name).count > 0;
}

Slice<const char *>
command_line_get_option_values(const Command_Line &self, const char *option_name)
{
	Slice<const char> option_name_slice = slice_from(option_name);
	for (const Command_Line_Option &option : self.options)
		if (option_name_slice == option.name)
			return slice_from(option.values.data, option.values.count);
	return Slice<const char *>{};
}