#pragma once

#include <core/defines.h>
#include "core/containers/array.h"
#include "core/containers/string.h"
#include "core/containers/hash_table.h"

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Support format specifiers.
	- [ ] Compile time check string format.
	- [ ] Should we provide format helpers for all of ours types here, or provide it in their files?
	- [ ] Check why the string capacity is bigger than it needs to be.
	- [ ] Use formatting in validate messages.
	- [ ] Cleanup.
*/

template <typename ...TArgs>
inline static String
format(const char *fmt, TArgs &&...args);

template <typename T>
requires (std::is_integral_v<T> && !std::is_floating_point_v<T>)
inline static String
format(T data, u8 base = 10, bool uppercase = false)
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
		temp[count++] = digits[(data % base)];
		data /= base;
	} while (data != 0);

	String buffer = string_init(memory::temp_allocator());

	if (base == 16)
	{
		string_append(buffer, '0');
		string_append(buffer, 'x');
		for (u64 i = 0; i < (base - count); ++i)
			string_append(buffer, '0');
	}
	else if (is_negative)
	{
		string_append(buffer, '-');
	}

	for (i64 i = count - 1; i >= 0; --i)
		string_append(buffer, temp[i]);

	return buffer;
}

template <typename T>
requires (std::is_floating_point_v<T>)
inline static String
format(T data)
{
	String buffer = string_init(memory::temp_allocator());

	if (data < 0)
	{
		string_append(buffer, '-');
		data = -data;
	}

	u64 integer = (u64)data;
	f64 fraction = data - integer;
	string_append(buffer, format((u64)integer));
	string_append(buffer, '.');

	//
	// NOTE:
	// Default precision is 6.
	//
	for (u64 i = 0; i < 6; ++i)
	{
		fraction *= 10;
		integer = (u64)fraction;
		string_append(buffer, format(integer));
		fraction = fraction - integer;
	}

	while (string_ends_with(buffer, '0'))
		string_remove_last(buffer);

	if (string_ends_with(buffer, '.'))
		string_remove_last(buffer);

	return buffer;
}

inline static String
format(bool data)
{
	String buffer = string_init(memory::temp_allocator());
	string_append(buffer, data ? string_literal("true") : string_literal("false")); // TODO: use c string.
	return buffer;
}

inline static String
format(char data)
{
	String buffer = string_init(memory::temp_allocator());
	string_append(buffer, data);
	return buffer;
}

template <typename T>
requires (std::is_pointer_v<T> && !is_c_string_v<T>)
inline static String
format(const T &data)
{
	return format((uptr)data, 16, true);
}

// TODO: Remove.
template <typename ...TArgs>
inline static void
string_append(String &self, const char *fmt, const TArgs &...args)
{
	validate(self.allocator, "[STRING]: Cannot append to a string literal.");
	string_append(self, format(fmt, args...));
}

// TODO: Remove.
template <typename ...TArgs>
inline static String
string_from(memory::Allocator *allocator, const char *fmt, const TArgs &...args)
{
	return string_copy(format(fmt, args...), allocator);
}

template <typename T>
requires (std::is_array_v<T> && !is_c_string_v<T>)
inline static String
format(const T &data)
{
	String buffer = string_init(memory::temp_allocator());

	if constexpr (is_char_array_v<T>)
	{
		u64 count = count_of(data);
		if (count > 0 && data[count - 1] == '\0')
			--count;

		String char_array_copy = string_init(memory::temp_allocator());
		string_resize(char_array_copy, count);
		for (u64 i = 0; i < count; ++i)
			char_array_copy[i] = data[i];

		string_append(buffer, format(char_array_copy.data));
	}
	else
	{
		string_append(buffer, format("[{}] {{ ", count_of(data)));
		for (u64 i = 0; i < count_of(data); ++i)
		{
			if (i != 0)
				string_append(buffer, ", ");
			string_append(buffer, format(data[i]));
		}
		string_append(buffer, " }}"); // TODO: This is formatted.
	}

	return buffer;
}

template <typename T>
inline static String
format(const Array<T> &data)
{
	String buffer = string_init(memory::temp_allocator());

	string_append(buffer, format("[{}] {{ ", data.count));
	for (u64 i = 0; i < data.count; ++i)
	{
		if (i != 0)
			string_append(buffer, ", ");
		string_append(buffer, format("{}", data[i]));
	}
	string_append(buffer, " }}"); // TODO: This is formatted.
	return buffer;
}

template <typename K, typename V>
inline static String
format(const Hash_Table<K, V> &data)
{
	String buffer = string_init(memory::temp_allocator());

	string_append(buffer, format("[{}] {{ ", data.count));
	u64 i = 0;
	for (const auto &[key, value] : data)
	{
		if (i != 0)
			string_append(buffer, ", ");
		string_append(buffer, format("{}: {}", key, value));
		++i;
	}
	string_append(buffer, " }}"); // TODO: This is formatted.
	return buffer;
}

template <typename ...TArgs>
inline static String
format(const String &fmt, TArgs &&...args)
{
	// 1. Validate that the format string is correct, and loop over all the replacement fields and match them against the arguments.
	// 2. Format each argument into the corresponding replacement field.
	// 3. Append the result string into the output buffer.

	if (string_is_empty(fmt))
		return string_literal("");

	String buffer = string_init(memory::temp_allocator());

	u64 replacement_field_count = 0;
	u64 replacement_field_largest_index = 0;
	for (u64 i = 0; i < fmt.count; ++i)
	{
		if (fmt[i] == '{')
		{
			validate((i + 1 < fmt.count && (fmt[i + 1] == '{' || fmt[i + 1] == '}')) || (i + 2 < fmt.count && fmt[i + 1] >= '0' && fmt[i + 1] <= '9' && fmt[i + 2] == '}'), "[FORMAT]: '{' must have a matching '{' or '}'.");

			if (fmt[i + 1] == '{')
			{
				string_append(buffer, '{');
				++i;
			}
			else if (fmt[i + 1] == '}')
			{
				if constexpr (sizeof...(args) > 0)
				{
					u64 index = 0;
					([&]<typename T>(const T &arg)
					{
						if (index == replacement_field_count)
						{
							if constexpr (is_char_array_v<T>)
							{
								u64 count = count_of(arg);
								if (count > 0 && arg[count - 1] == '\0')
									--count;
								for (u64 i = 0; i < count; ++i)
									string_append(buffer, arg[i]);
							}
							else if constexpr (std::is_same_v<T, String>)
							{
								for (u64 i = 0; i < arg.count; ++i)
									string_append(buffer, arg[i]);
							}
							else if constexpr (is_c_string_v<T>)
							{
								for (u64 i = 0; i < ::strlen(arg); ++i)
									string_append(buffer, arg[i]);
							}
							else
							{
								string_append(buffer, format(arg));
							}
						}
						++index;
					}(args), ...);
				}

				++i;
				++replacement_field_count;
			}
			else if (fmt[i + 1] >= '0' && fmt[i + 1] <= '9')
			{
				// TODO: Doesn't handle more than 10 indices.
				validate(fmt[i + 2] == '}', "[FORMAT]: Missing '}' for indexed replacement field.");

				u64 replacement_field_index = fmt[i + 1] - '0';
				if (replacement_field_index > replacement_field_largest_index)
					replacement_field_largest_index = replacement_field_index;

				validate(replacement_field_index < sizeof...(args), "[FORMAT]: Replacement field index exceeds the total number of arguments passed.");

				if constexpr (sizeof...(args) > 0)
				{
					u64 index = 0;
					([&]<typename T>(const T &arg)
					{
						if (index == replacement_field_index)
						{
							if constexpr (is_char_array_v<T>)
							{
								u64 count = count_of(arg);
								if (count > 0 && arg[count - 1] == '\0')
									--count;
								for (u64 i = 0; i < count; ++i)
									string_append(buffer, arg[i]);
							}
							else if constexpr (std::is_same_v<T, String>)
							{
								for (u64 i = 0; i < arg.count; ++i)
									string_append(buffer, arg[i]);
							}
							else if constexpr (is_c_string_v<T>)
							{
								for (u64 i = 0; i < ::strlen(arg); ++i)
									string_append(buffer, arg[i]);
							}
							else
							{
								string_append(buffer, format(arg));
							}
						}
						++index;
					}(args), ...);
				}

				i += 2;
			}
		}
		else if (fmt[i] == '}')
		{
			validate(i + 1 < fmt.count && fmt[i + 1] == '}', "[FORMAT]: '}' must have a matching '}'.");

			string_append(buffer, '}');
			++i;
		}
		else
		{
			string_append(buffer, fmt[i]);
		}
	}

	validate(replacement_field_count == 0 || replacement_field_largest_index == 0, "[FORMATTER]: Cannot mix between automatic and manual replacement field indexing.");
	validate(replacement_field_count == sizeof...(args) || (replacement_field_largest_index + 1) == sizeof...(args), "[FORMAT]: Replacement field count does not match argument count.");

	return buffer;
}

// TODO: Pass as reference and check if char[] won't get decayed into pointers.
template <typename ...TArgs>
inline static String
format(const char *fmt, TArgs &&...args)
{
	return format(string_literal(fmt), std::forward<TArgs>(args)...);
}

template <typename T>
requires (!std::is_same_v<T, String>)
inline static String
to_string(const T &data)
{
	return format("{}", data);
}