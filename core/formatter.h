#pragma once

#include <core/defines.h>
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

#include <stdlib.h>

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
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

enum Format_Alignment
{
	FORMAT_ALIGNMENT_NONE,
	FORMAT_ALIGNMENT_LEFT,
	FORMAT_ALIGNMENT_RIGHT,
	FORMAT_ALIGNMENT_CENTER,
};

struct Format_Options
{
	Format_Specifier specifier = FORMAT_SPECIFIER_NONE;
	Format_Alignment alignment = FORMAT_ALIGNMENT_NONE;
	u32 width = 0;
	u32 precision = 6;
	bool zero_pad = false;
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

inline static void
format_apply_width_alignment(Formatter &self, const String &content, const Format_Options &options)
{
	if (options.width == 0 || content.count >= options.width)
	{
		string_append(self.buffer, content);
		return;
	}

	u64 padding = options.width - content.count;

	// Special handling for zero-padding with negative numbers or prefixes
	if (options.zero_pad && (options.alignment == FORMAT_ALIGNMENT_NONE || options.alignment == FORMAT_ALIGNMENT_RIGHT))
	{
		// Check if content starts with a sign or prefix
		bool has_sign = content.count > 0 && (content[0] == '-' || content[0] == '+');
		bool has_prefix = content.count > 1 && content[0] == '0' && (content[1] == 'x' || content[1] == 'X' ||
																	   content[1] == 'b' || content[1] == 'B' ||
																	   content[1] == 'o' || content[1] == 'O');

		if (has_sign)
		{
			// Output sign first
			string_append(self.buffer, content[0]);

			// Check if there's also a prefix after the sign (e.g., "-0x")
			if (content.count > 3 && content[1] == '0' && (content[2] == 'x' || content[2] == 'X' ||
															content[2] == 'b' || content[2] == 'B' ||
															content[2] == 'o' || content[2] == 'O'))
			{
				// Output prefix
				string_append(self.buffer, content[1]);
				string_append(self.buffer, content[2]);
				// Output padding
				for (u64 i = 0; i < padding; ++i)
					string_append(self.buffer, '0');
				// Output rest of content
				for (u64 i = 3; i < content.count; ++i)
					string_append(self.buffer, content[i]);
			}
			else
			{
				// Output padding
				for (u64 i = 0; i < padding; ++i)
					string_append(self.buffer, '0');
				// Output rest of content (skip the sign we already added)
				for (u64 i = 1; i < content.count; ++i)
					string_append(self.buffer, content[i]);
			}
		}
		else if (has_prefix)
		{
			// Output prefix first (e.g., "0x")
			string_append(self.buffer, content[0]);
			string_append(self.buffer, content[1]);
			// Output padding
			for (u64 i = 0; i < padding; ++i)
				string_append(self.buffer, '0');
			// Output rest of content
			for (u64 i = 2; i < content.count; ++i)
				string_append(self.buffer, content[i]);
		}
		else
		{
			// No sign or prefix, just pad normally
			for (u64 i = 0; i < padding; ++i)
				string_append(self.buffer, '0');
			string_append(self.buffer, content);
		}
		return;
	}

	// Regular padding (space padding or non-right alignment)
	char pad_char = ' '; // Always use space for non-zero padding or non-right alignment

	if (options.alignment == FORMAT_ALIGNMENT_LEFT)
	{
		// Left align: content then padding
		string_append(self.buffer, content);
		for (u64 i = 0; i < padding; ++i)
			string_append(self.buffer, pad_char);
	}
	else if (options.alignment == FORMAT_ALIGNMENT_CENTER)
	{
		// Center align: padding/2, content, padding/2
		u64 left_pad = padding / 2;
		u64 right_pad = padding - left_pad;
		for (u64 i = 0; i < left_pad; ++i)
			string_append(self.buffer, pad_char);
		string_append(self.buffer, content);
		for (u64 i = 0; i < right_pad; ++i)
			string_append(self.buffer, pad_char);
	}
	else // FORMAT_ALIGNMENT_RIGHT or NONE (default right for numbers)
	{
		// Right align: padding then content
		for (u64 i = 0; i < padding; ++i)
			string_append(self.buffer, pad_char);
		string_append(self.buffer, content);
	}
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
		is_negative = (data < 0);
		if (is_negative)
			data = (T)(-data);
	}

	char temp[64] = {};
	u64 count = 0;
	do
	{
		temp[count++] = digits[(uptr)(data % base)];
		data = (T)(data / base);
	} while (data != 0);

	// Always emit sign first (if negative) so prefix follows the sign (C++-style)
	if (is_negative)
		string_append(self.buffer, '-');

	// Add prefix based on base (prefix comes after sign)
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
	// for base == 10 there's no prefix; sign already emitted

	for (i64 i = count - 1; i >= 0; --i)
		string_append(self.buffer, temp[i]);

	return self.buffer;
}

template <typename T>
requires (std::is_integral_v<T> && !std::is_floating_point_v<T>)
inline static String
format(Formatter &self, T data, const Format_Options &options)
{
	// Create temporary formatter for content
	Formatter temp = formatter_init(self.buffer.allocator);
	DEFER(formatter_deinit(temp));

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

	format(temp, data, base, uppercase);
	format_apply_width_alignment(self, temp.buffer, options);
	return self.buffer;
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

template <typename T>
requires (std::is_floating_point_v<T>)
inline static String
format(Formatter &self, T data, const Format_Options &options)
{
	// Create temporary formatter for content
	Formatter temp = formatter_init(self.buffer.allocator);
	DEFER(formatter_deinit(temp));

	format(temp, data, options.precision, options.remove_trailing_zeros);
	format_apply_width_alignment(self, temp.buffer, options);
	return self.buffer;
}

inline static String
format(Formatter &self, bool data)
{
	string_append(self.buffer, data ? "true" : "false");
	return self.buffer;
}

inline static String
format(Formatter &self, bool data, const Format_Options &options)
{
	Formatter temp = formatter_init(self.buffer.allocator);
	DEFER(formatter_deinit(temp));

	format(temp, data);
	format_apply_width_alignment(self, temp.buffer, options);
	return self.buffer;
}

inline static String
format(Formatter &self, char data)
{
	string_append(self.buffer, data);
	return self.buffer;
}

inline static String
format(Formatter &self, char data, const Format_Options &options)
{
	if (options.specifier == FORMAT_SPECIFIER_CHAR_LOWER ||
		options.specifier == FORMAT_SPECIFIER_CHAR_UPPER ||
		options.specifier == FORMAT_SPECIFIER_NONE)
	{
		// Format as character with width/alignment
		Formatter temp = formatter_init(self.buffer.allocator);
		DEFER(formatter_deinit(temp));

		if (options.specifier == FORMAT_SPECIFIER_CHAR_LOWER)
			string_append(temp.buffer, (char)(data | 0x20)); // to lowercase
		else if (options.specifier == FORMAT_SPECIFIER_CHAR_UPPER)
			string_append(temp.buffer, (char)(data & ~0x20)); // to uppercase
		else
			string_append(temp.buffer, data);

		format_apply_width_alignment(self, temp.buffer, options);
	}
	else
	{
		// Format as integer
		format(self, (u8)data, options);
	}
	return self.buffer;
}

template <typename T>
requires (std::is_pointer_v<T> && !is_c_string_v<T>)
inline static String
format(Formatter &self, const T &data)
{
	return format(self, (uptr)data, 16, false);
}

template <typename T>
requires (std::is_pointer_v<T> && !is_c_string_v<T>)
inline static String
format(Formatter &self, const T &data, const Format_Options &options)
{
	Formatter temp = formatter_init(self.buffer.allocator);
	DEFER(formatter_deinit(temp));

	bool uppercase = (options.specifier == FORMAT_SPECIFIER_POINTER_UPPER);
	format(temp, (uptr)data, 16, uppercase);
	format_apply_width_alignment(self, temp.buffer, options);
	return self.buffer;
}

inline static String
format(Formatter &self, const char *data)
{
	if (data == nullptr)
		return self.buffer;
	string_append(self.buffer, string_literal(data));
	return self.buffer;
}

inline static String
format(Formatter &self, const char *data, const Format_Options &options)
{
	if (data == nullptr)
		return self.buffer;

	Formatter temp = formatter_init(self.buffer.allocator);
	DEFER(formatter_deinit(temp));

	string_append(temp.buffer, string_literal(data));
	format_apply_width_alignment(self, temp.buffer, options);
	return self.buffer;
}

inline static String
format(Formatter &self, const String &data)
{
	string_append(self.buffer, data);
	return self.buffer;
}

inline static String
format(Formatter &self, const String &data, const Format_Options &options)
{
	format_apply_width_alignment(self, data, options);
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
	field.options.alignment = FORMAT_ALIGNMENT_NONE;
	field.options.width = 0;
	field.options.precision = 6;
	field.options.zero_pad = false;
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

		// Parse alignment (optional)
		if (i < fmt.count)
		{
			if (fmt[i] == '<')
			{
				field.options.alignment = FORMAT_ALIGNMENT_LEFT;
				++i;
			}
			else if (fmt[i] == '>')
			{
				field.options.alignment = FORMAT_ALIGNMENT_RIGHT;
				++i;
			}
			else if (fmt[i] == '^')
			{
				field.options.alignment = FORMAT_ALIGNMENT_CENTER;
				++i;
			}
		}

		// Parse zero-padding (optional)
		if (i < fmt.count && fmt[i] == '0')
		{
			field.options.zero_pad = true;
			++i;
		}

		// Parse width (optional)
		if (i < fmt.count && fmt[i] >= '0' && fmt[i] <= '9')
		{
			char *end = nullptr;
			field.options.width = ::strtoul(&fmt[i], &end, 10);
			i64 length = end - &fmt[i];
			i += (u32)length;
		}

		// Parse type specifier (optional)
		if (i < fmt.count)
		{
			switch (fmt[i])
			{
				case 'd': case 'D':
					field.options.specifier = (fmt[i] == 'd') ? FORMAT_SPECIFIER_DECIMAL_LOWER : FORMAT_SPECIFIER_DECIMAL_UPPER;
					++i;
					break;
				case 'x': case 'X':
					field.options.specifier = (fmt[i] == 'x') ? FORMAT_SPECIFIER_HEX_LOWER : FORMAT_SPECIFIER_HEX_UPPER;
					++i;
					break;
				case 'b': case 'B':
					field.options.specifier = (fmt[i] == 'b') ? FORMAT_SPECIFIER_BINARY_LOWER : FORMAT_SPECIFIER_BINARY_UPPER;
					++i;
					break;
				case 'o': case 'O':
					field.options.specifier = (fmt[i] == 'o') ? FORMAT_SPECIFIER_OCTAL_LOWER : FORMAT_SPECIFIER_OCTAL_UPPER;
					++i;
					break;
				case 'p': case 'P':
					field.options.specifier = (fmt[i] == 'p') ? FORMAT_SPECIFIER_POINTER_LOWER : FORMAT_SPECIFIER_POINTER_UPPER;
					++i;
					break;
				case 'c': case 'C':
					field.options.specifier = (fmt[i] == 'c') ? FORMAT_SPECIFIER_CHAR_LOWER : FORMAT_SPECIFIER_CHAR_UPPER;
					++i;
					break;
				default:
					break;
			}
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
				format(self, data, options);
			}
			else if constexpr (is_char_array_v<T>)
			{
				Formatter temp = formatter_init(self.buffer.allocator);
				DEFER(formatter_deinit(temp));
				u64 count = count_of(data);
				if (count > 0 && data[count - 1] == '\0')
					--count;
				for (u64 i = 0; i < count; ++i)
					string_append(temp.buffer, data[i]);
				format_apply_width_alignment(self, temp.buffer, options);
			}
			else if constexpr (std::is_same_v<T, String>)
			{
				format(self, data, options);
			}
			else if constexpr (is_c_string_v<T>)
			{
				format(self, data, options);
			}
			else if constexpr (std::is_pointer_v<T> && !is_c_string_v<T>)
			{
				format(self, data, options);
			}
			else if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T, bool>)
			{
				format(self, data, options);
			}
			else if constexpr (std::is_floating_point_v<T>)
			{
				format(self, data, options);
			}
			else if constexpr (std::is_same_v<T, bool>)
			{
				format(self, data, options);
			}
			else
			{
				Formatter temp = formatter_init(self.buffer.allocator);
				DEFER(formatter_deinit(temp));
				format(temp, data);
				format_apply_width_alignment(self, temp.buffer, options);
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