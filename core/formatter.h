#pragma once

#include <core/defines.h>
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

#include <stdlib.h>

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Support more format specifiers (width, padding, alignment).
	- [ ] Compile time check string format.
	- [ ] Use formatting in validate messages.
	- [ ] Cleanup.
*/

enum Format_Specifier
{
	FORMAT_SPECIFIER_NONE,
	FORMAT_SPECIFIER_DECIMAL_LOWER,
	FORMAT_SPECIFIER_DECIMAL_UPPER,
	FORMAT_SPECIFIER_HEX_LOWER,
	FORMAT_SPECIFIER_HEX_UPPER,
	FORMAT_SPECIFIER_BINARY_LOWER,
	FORMAT_SPECIFIER_BINARY_UPPER,
	FORMAT_SPECIFIER_OCTAL_LOWER,
	FORMAT_SPECIFIER_OCTAL_UPPER,
	FORMAT_SPECIFIER_POINTER_LOWER,
	FORMAT_SPECIFIER_POINTER_UPPER,
	FORMAT_SPECIFIER_CHAR_LOWER,
	FORMAT_SPECIFIER_CHAR_UPPER,
};

struct Format_Options
{
	Format_Specifier specifier = FORMAT_SPECIFIER_NONE;
	u32 precision = 6;
	bool remove_trailing_zeros = true;
};

struct Formatter
{
	String buffer;
};

inline static Formatter
formatter_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Formatter {
		.buffer = string_init(allocator)
	};
}

inline static void
formatter_deinit(Formatter &self)
{
	string_deinit(self.buffer);
	self = Formatter{};
}

inline static void
formatter_clear(Formatter &self)
{
	string_clear(self.buffer);
}

template <typename T>
requires (std::is_integral_v<T> && !std::is_floating_point_v<T>)
inline static String
format(Formatter &self, T data, u8 base = 10, bool uppercase = false)
{
	const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

	bool is_negative = false;
	if constexpr (std::is_signed_v<T>)
	{
		is_negative = data < 0;
		if (is_negative)
			data = -data;
	}

	char temp[64] = {};
	u64 count = 0;
	do
	{
		temp[count++] = digits[data % base];
		data /= base;
	} while (data != 0);

	// Add prefix based on base
	if (base == 16)
	{
		string_append(self.buffer, '0');
		string_append(self.buffer, uppercase ? 'X' : 'x');
		// Pad to at least 2 digits for hex
		if (count < 2)
		{
			for (u64 i = count; i < 2; ++i)
				string_append(self.buffer, '0');
		}
	}
	else if (base == 2)
	{
		string_append(self.buffer, '0');
		string_append(self.buffer, uppercase ? 'B' : 'b');
	}
	else if (base == 8)
	{
		string_append(self.buffer, '0');
		string_append(self.buffer, uppercase ? 'O' : 'o');
	}
	else if (is_negative)
	{
		string_append(self.buffer, '-');
	}

	for (i64 i = count - 1; i >= 0; --i)
		string_append(self.buffer, temp[i]);

	return self.buffer;
}

template <typename T>
requires (std::is_integral_v<T> && !std::is_floating_point_v<T>)
inline static String
format(Formatter &self, T data, const Format_Options &options)
{
	u8 base = 10;
	bool uppercase = false;

	switch (options.specifier)
	{
		case FORMAT_SPECIFIER_DECIMAL_LOWER:
			base = 10;
			uppercase = false;
			break;
		case FORMAT_SPECIFIER_DECIMAL_UPPER:
			base = 10;
			uppercase = true;
			break;
		case FORMAT_SPECIFIER_HEX_LOWER:
			base = 16;
			uppercase = false;
			break;
		case FORMAT_SPECIFIER_HEX_UPPER:
			base = 16;
			uppercase = true;
			break;
		case FORMAT_SPECIFIER_BINARY_LOWER:
			base = 2;
			uppercase = false;
			break;
		case FORMAT_SPECIFIER_BINARY_UPPER:
			base = 2;
			uppercase = true;
			break;
		case FORMAT_SPECIFIER_OCTAL_LOWER:
			base = 8;
			uppercase = false;
			break;
		case FORMAT_SPECIFIER_OCTAL_UPPER:
			base = 8;
			uppercase = true;
			break;
		case FORMAT_SPECIFIER_POINTER_LOWER:
			base = 16;
			uppercase = false;
			break;
		case FORMAT_SPECIFIER_POINTER_UPPER:
			base = 16;
			uppercase = true;
			break;
		default:
			base = 10;
			break;
	}

	return format(self, data, base, uppercase);
}

template <typename T>
requires (std::is_floating_point_v<T>)
inline static String
format(Formatter &self, T data, u32 precision = 6, bool remove_trailing_zeros = true)
{
	if (data < 0)
	{
		string_append(self.buffer, '-');
		data = -data;
	}

	u64 integer = (u64)data;
	f64 fraction = data - integer;
	format(self, (u64)integer);
	string_append(self.buffer, '.');

	for (u64 i = 0; i < precision; ++i)
	{
		fraction *= 10;
		integer = (u64)fraction;
		format(self, integer);
		fraction = fraction - integer;
	}

	if (remove_trailing_zeros)
		while (string_ends_with(self.buffer, '0'))
			string_remove_last(self.buffer);

	if (string_ends_with(self.buffer, '.'))
		string_remove_last(self.buffer);

	return self.buffer;
}

inline static String
format(Formatter &self, bool data)
{
	string_append(self.buffer, data ? "true" : "false");
	return self.buffer;
}

template <typename T>
requires (std::is_array_v<T> && !is_char_array_v<T> && !is_c_string_v<T>)
inline static String
format(Formatter &self, const T &data)
{
	format(self, "[{}] {{ ", count_of(data));
	for (u64 i = 0; i < count_of(data); ++i)
	{
		if (i != 0)
			string_append(self.buffer, ", ");
		format(self, data[i]);
	}
	string_append(self.buffer, " }");

	return self.buffer;
}

template <typename T>
inline static String
format(Formatter &self, const Array<T> &data)
{
	format(self, "[{}] {{ ", data.count);
	for (u64 i = 0; i < data.count; ++i)
	{
		if (i != 0)
			string_append(self.buffer, ", ");
		format(self, "{}", data[i]);
	}
	string_append(self.buffer, " }");
	return self.buffer;
}

template <typename K, typename V>
inline static String
format(Formatter &self, const Hash_Table<K, V> &data)
{
	format(self, "[{}] {{ ", data.count);
	u64 i = 0;
	for (const auto &[key, value] : data)
	{
		if (i != 0)
			string_append(self.buffer, ", ");
		format(self, "{}: {}", key, value);
		++i;
	}
	string_append(self.buffer, " }");
	return self.buffer;
}

// --- Format specifier parsing ---

struct Format_Field
{
	u32 index;
	Format_Options options;
	bool has_index;
};

inline static Format_Field
parse_format_field(const String &fmt, u32 &i)
{
	Format_Field field = {};
	field.options.specifier = FORMAT_SPECIFIER_NONE;
	field.options.precision = 6;
	field.options.remove_trailing_zeros = true;
	field.has_index = false;

	// Check for indexed field {0}, {1}, etc.
	if (i + 1 < fmt.count && fmt[i + 1] >= '0' && fmt[i + 1] <= '9')
	{
		char *end = nullptr;
		field.index = ::strtoul(&fmt[i + 1], &end, 10);
		field.has_index = true;
		i64 length = end - &fmt[i + 1];
		i += (u32)length + 1;
	}
	else
	{
		++i; // Move past '{'
	}

	// Check for format specifier ':'
	if (i < fmt.count && fmt[i] == ':')
	{
		++i; // Move past ':'
		if (i < fmt.count)
		{
			switch (fmt[i])
			{
				case 'd': field.options.specifier = FORMAT_SPECIFIER_DECIMAL_LOWER; break;
				case 'D': field.options.specifier = FORMAT_SPECIFIER_DECIMAL_UPPER; break;
				case 'x': field.options.specifier = FORMAT_SPECIFIER_HEX_LOWER; break;
				case 'X': field.options.specifier = FORMAT_SPECIFIER_HEX_UPPER; break;
				case 'b': field.options.specifier = FORMAT_SPECIFIER_BINARY_LOWER; break;
				case 'B': field.options.specifier = FORMAT_SPECIFIER_BINARY_UPPER; break;
				case 'o': field.options.specifier = FORMAT_SPECIFIER_OCTAL_LOWER; break;
				case 'O': field.options.specifier = FORMAT_SPECIFIER_OCTAL_UPPER; break;
				case 'p': field.options.specifier = FORMAT_SPECIFIER_POINTER_LOWER; break;
				case 'P': field.options.specifier = FORMAT_SPECIFIER_POINTER_UPPER; break;
				case 'c': field.options.specifier = FORMAT_SPECIFIER_CHAR_LOWER; break;
				case 'C': field.options.specifier = FORMAT_SPECIFIER_CHAR_UPPER; break;
				default: validate("[FORMAT]: Unsupported format specifier."); break;
			}
			++i; // Move past specifier
		}
	}

	validate(i < fmt.count && fmt[i] == '}', "[FORMAT]: Missing '}' in format string.");
	return field;
}

template <typename ...TArgs>
inline static String
format(Formatter &self, const String &fmt, TArgs &&...args)
{
	constexpr auto append_field_data = []<typename T>(Formatter &self, const T &data, u32 &argument_index, u32 target_index, const Format_Options &options) {
		if (argument_index == target_index)
		{
			if constexpr (std::is_same_v<T, char>)
			{
				if (options.specifier == FORMAT_SPECIFIER_CHAR_LOWER)
					string_append(self.buffer, data | 0x20);
				else if (options.specifier == FORMAT_SPECIFIER_CHAR_UPPER)
					string_append(self.buffer, data & ~0x20);
				else if (options.specifier != FORMAT_SPECIFIER_NONE)
					format(self, data, options);
				else
					string_append(self.buffer, data);
			}
			else if constexpr (is_char_array_v<T>)
			{
				u64 count = count_of(data);
				if (count > 0 && data[count - 1] == '\0')
					--count;
				for (u64 i = 0; i < count; ++i)
					string_append(self.buffer, data[i]);
			}
			else if constexpr (std::is_same_v<T, String>)
			{
				string_append(self.buffer, data);
			}
			else if constexpr (is_c_string_v<T>)
			{
				string_append(self.buffer, string_literal(data));
			}
			else if constexpr (std::is_pointer_v<T>)
			{
				if (options.specifier != FORMAT_SPECIFIER_NONE)
					format(self, (uptr)data, options);
				else
					format(self, data);
			}
			else if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T, bool>)
			{
				if (options.specifier != FORMAT_SPECIFIER_NONE)
					format(self, data, options);
				else
					format(self, data);
			}
			else
			{
				format(self, data);
			}
		}
		++argument_index;
	};

	constexpr auto is_allocator = []<typename T>(const T &) -> bool {
		return std::is_base_of_v<memory::Allocator, T> || std::is_same_v<T, memory::Allocator *>;
	};

	if (string_is_empty(fmt))
		return string_literal("");

	// Count arguments (excluding trailing allocator)
	u32 argument_count = sizeof...(args);
	if constexpr (sizeof...(args) > 0)
	{
		[[maybe_unused]] u32 argument_index = 0;
		([&]() {
			if (argument_index == argument_count - 1 && is_allocator(args))
				--argument_count;
			++argument_index;
		}(), ...);
	}

	u32 auto_index = 0;
	bool uses_manual_indexing = false;
	bool uses_auto_indexing = false;

	for (u32 i = 0; i < fmt.count; ++i)
	{
		if (fmt[i] == '{')
		{
			validate(i + 1 < fmt.count, "[FORMAT]: Unexpected end after '{'.");

			// Handle escaped '{'
			if (fmt[i + 1] == '{')
			{
				string_append(self.buffer, '{');
				++i;
				continue;
			}

			// Parse format field
			Format_Field field = parse_format_field(fmt, i);

			// Determine which index to use
			u32 target_index;
			if (field.has_index)
			{
				uses_manual_indexing = true;
				target_index = field.index;
				validate(target_index < argument_count, "[FORMAT]: Replacement field index exceeds argument count.");
			}
			else
			{
				uses_auto_indexing = true;
				target_index = auto_index++;
			}

			// Append the argument
			if constexpr (sizeof...(args) > 0)
			{
				u32 index = 0;
				(append_field_data(self, args, index, target_index, field.options), ...);
			}
		}
		else if (fmt[i] == '}')
		{
			validate(i + 1 < fmt.count && fmt[i + 1] == '}', "[FORMAT]: Unmatched '}'.");
			string_append(self.buffer, '}');
			++i;
		}
		else
		{
			string_append(self.buffer, fmt[i]);
		}
	}

	validate(!uses_manual_indexing || !uses_auto_indexing, "[FORMAT]: Cannot mix automatic and manual indexing.");
	validate(auto_index == argument_count || uses_manual_indexing, "[FORMAT]: Argument count mismatch.");

	return self.buffer;
}

template <typename ...TArgs>
inline static String
format(Formatter &self, const char *fmt, TArgs &&...args)
{
	return format(self, string_literal(fmt), std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static String
format(const String &fmt, TArgs &&...args)
{
	[[maybe_unused]] constexpr auto set_allocator = []<typename T>(memory::Allocator *&allocator, const T &data, u32 &argument_index, u32 argument_count) {
		if constexpr (std::is_base_of_v<memory::Allocator, T> || std::is_same_v<T, memory::Allocator *>)
			if (argument_index == argument_count - 1)
				allocator = data;
		++argument_index;
	};

	memory::Allocator *allocator = memory::heap_allocator();
	[[maybe_unused]] u32 argument_index = 0;
	(set_allocator(allocator, args, argument_index, sizeof...(args)), ...);

	Formatter self = formatter_init(allocator);
	DEFER(self = Formatter{});
	return format(self, fmt, std::forward<TArgs>(args)...);
}

template <typename ...TArgs>
inline static String
format(const char *fmt, TArgs &&...args)
{
	return format(string_literal(fmt), std::forward<TArgs>(args)...);
}

template <typename T>
requires (!std::is_same_v<T, String>)
inline static String
to_string(const T &data, memory::Allocator *allocator = memory::heap_allocator())
{
	Formatter self = formatter_init(allocator);
	DEFER(self = Formatter{});
	return format(self, "{}", data);
}