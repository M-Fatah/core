#pragma once

#include <core/defines.h>
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

#include <stdlib.h>

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Support format specifiers.
	- [ ] Compile time check string format.
	- [ ] Use formatting in validate messages.
	- [ ] Experiment with std::tuple for the TArgs.
	- [ ] Cleanup.
	- [ ] Properly format arrays, ...etc like fmt lib.
*/

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

	if (base == 16)
	{
		string_append(self.buffer, '0');
		string_append(self.buffer, 'x');
		for (u64 i = 0; i < (base - count); ++i)
			string_append(self.buffer, '0');
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

inline static String
format(Formatter &self, char data)
{
	string_append(self.buffer, data);
	return self.buffer;
}

template <typename T>
requires (std::is_pointer_v<T> && !is_c_string_v<T>)
inline static String
format(Formatter &self, const T &data)
{
	return format(self, (uptr)data, 16, true);
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

template <typename ...TArgs>
inline static String
format(Formatter &self, const String &fmt, TArgs &&...args)
{
	constexpr auto append_field_data = []<typename T>(Formatter &self, const T &data, u32 &argument_index, u32 replacement_field_index) {
		if (argument_index == replacement_field_index)
		{
			if constexpr (std::is_same_v<T, char>)
			{
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
			else
			{
				format(self, data);
			}
		}
		++argument_index;
	};

	constexpr auto set_argument_count = []<typename T>(const T &, u32 &argument_index, u32 &argument_count) {
		if constexpr (std::is_base_of_v<memory::Allocator, T> || std::is_same_v<T, memory::Allocator *>)
			if (argument_index == argument_count - 1)
				--argument_count;
		++argument_index;
	};

	if (string_is_empty(fmt))
		return string_literal("");

	u32 argument_count = sizeof...(args);
	if constexpr (sizeof...(args) > 0)
	{
		u32 argument_index = 0;
		(set_argument_count(args, argument_index, argument_count), ...);
	}

	u32 replacement_field_count = 0;
	u32 replacement_field_largest_index = 0;
	for (u32 i = 0; i < fmt.count; ++i)
	{
		if (fmt[i] == '{')
		{
			validate(i + 1 < fmt.count && ((fmt[i + 1] == '{' || fmt[i + 1] == '}') || (fmt[i + 1] >= '0' && fmt[i + 1] <= '9')), "[FORMAT]: '{' must have a matching '{' or '}'.");

			if (fmt[i + 1] == '{')
			{
				string_append(self.buffer, '{');
				++i;
			}
			else if (fmt[i + 1] == '}')
			{
				if constexpr (sizeof...(args) > 0)
				{
					u32 index = 0;
					(append_field_data(self, args, index, replacement_field_count), ...);
				}

				++i;
				++replacement_field_count;
			}
			else if (fmt[i + 1] >= '0' && fmt[i + 1] <= '9')
			{
				// TODO: Cleanup.
				char *end = nullptr;

				u32 replacement_field_index = ::strtoul(&fmt[i + 1], &end, 10);
				if (replacement_field_index > replacement_field_largest_index)
				replacement_field_largest_index = replacement_field_index;

				validate(replacement_field_index < argument_count, "[FORMAT]: Replacement field index exceeds the total number of arguments passed.");

				if constexpr (sizeof...(args) > 0)
				{
					u32 index = 0;
					(append_field_data(self, args, index, replacement_field_index), ...);
				}

				i64 length = end - &fmt[i + 1];

				i += (u32)length;
				validate(fmt[i + 1] == '}', "[FORMAT]: Missing '}' for indexed replacement field.");
				i += 1;
			}
		}
		else if (fmt[i] == '}')
		{
			validate(i + 1 < fmt.count && fmt[i + 1] == '}', "[FORMAT]: '}' must have a matching '}'.");

			string_append(self.buffer, '}');
			++i;
		}
		else
		{
			string_append(self.buffer, fmt[i]);
		}
	}

	validate(replacement_field_count == 0 || replacement_field_largest_index == 0, "[FORMATTER]: Cannot mix between automatic and manual replacement field indexing.");
	validate(replacement_field_count == argument_count || (replacement_field_largest_index + 1) == argument_count, "[FORMAT]: Replacement field count does not match argument count.");

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
	constexpr auto set_allocator = []<typename T>(memory::Allocator *&allocator, const T &data, u32 &argument_index, u32 argument_count) {
		if constexpr (std::is_base_of_v<memory::Allocator, T> || std::is_same_v<T, memory::Allocator *>)
			if (argument_index == argument_count - 1)
				allocator = data;
		++argument_index;
	};

	memory::Allocator *allocator = memory::heap_allocator();
	u32 argument_index = 0;
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