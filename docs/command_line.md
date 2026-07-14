# Command Line

`core/command_line.h` provides a small descriptor-driven parser for `argc` / `argv` style command lines.

It is meant for tools, compilers, game launchers, tests, and examples that need predictable command-line parsing without STL containers, exceptions, callbacks, or hidden string copies.

---

## Overview

The parser takes:

- the raw command-line arguments
- known option descriptions
- an optional allocator for the parser-owned arrays

It returns a `Command_Line` value containing:

- `executable`: the first argument
- parsed options
- positional arguments
- parse errors

The executable, positional arguments, parsed option names, option values, and error arguments are stored as `const char *` values. The parser owns the arrays used to group the parsed result, so the original argument strings and option description names must outlive the `Command_Line`.

---

## Option Descriptions

```cpp
static Command_Line_Option_Desc option_descs[] = {
	{"help", 'h'},
	{"output", 'o', true},
	{"include", 'I', true},
	{"threads", 'j', true},
};
```

Options are flags by default. Set `requires_value` to `true` for options that require a non-empty value.

Initialize the parser with the argument array, argument count, and option descriptions:

```cpp
Command_Line command_line = command_line_init(argv, argc, option_descs);
```

The allocator defaults to `memory::heap_allocator()`.

Pass another allocator as the final argument when needed:

```cpp
Command_Line command_line = command_line_init(argv, argc, option_descs, allocator);
```

---

## Supported Forms

```text
tool --help
tool --output build/game.exe
tool --output=build/game.exe
tool -o build/game.exe
tool -Icore
tool -I deps/core
tool -- -input-that-starts-with-dash
```

A required option consumes the following argument even when it begins with `-`:

```text
tool --output --verbose
```

Here `--verbose` is the value of `output`. Use an attached value such as `--output=-file` when that makes the intent clearer. A standalone `--` ends option parsing only when it is reached as its own argument; every following argument is positional. A single `-` is also positional.

Repeated options are preserved:

```text
tool -Icore -Ideps -Igenerated
```

`command_line_get_option_values()` returns every occurrence in argument order. Flags contribute a `nullptr` value, so the returned slice count also records how many times a flag was supplied.

Positionals are arguments that are not parsed as options:

```text
compiler main.cpp math.cpp --output game.exe
```

Here `main.cpp` and `math.cpp` are positionals.

---

## Example

```cpp
#include <core/command_line.h>
#include <core/defer.h>

int
main(int argc, char **argv)
{
	static Command_Line_Option_Desc option_descs[] = {
		{"help", 'h'},
		{"output", 'o', true},
		{"include", 'I', true},
		{"threads", 'j', true},
	};

	Command_Line command_line = command_line_init(argv, argc, option_descs);
	DEFER(command_line_deinit(command_line));

	if (command_line_has_errors(command_line))
	{
		for (Command_Line_Error error : command_line.errors)
		{
			// Report error.code and error.argument through your diagnostic path.
		}
		return 1;
	}

	if (command_line_has_option(command_line, "help"))
	{
		// Print help text.
		return 0;
	}

	Slice<const char *> output_values =
		command_line_get_option_values(command_line, "output");
	if (output_values.count > 0)
	{
		const char *output = output_values[0];
		// Use output.
	}

	for (const char *include_path :
		command_line_get_option_values(command_line, "include"))
	{
		// Use include_path.
	}

	for (const char *input : command_line.positionals)
	{
		// Use input file/project/target.
	}

	return 0;
}
```

---

## Errors

`command_line_has_errors()` returns `true` when any parse error was collected.

```cpp
Slice<const Command_Line_Error> errors = slice_from(command_line.errors);
```

Possible error codes:

```cpp
COMMAND_LINE_ERROR_UNKNOWN_OPTION
COMMAND_LINE_ERROR_MISSING_VALUE
COMMAND_LINE_ERROR_UNEXPECTED_VALUE
```

Each error stores the exact argument that caused it, such as `--unknown` or `--output=`. The parser records errors and keeps parsing the remaining arguments where possible. Callers choose how to format and report each error using its `code` and `argument` fields.