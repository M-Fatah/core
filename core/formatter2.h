#pragma once

#include "core/result.h"
#include "core/containers/string.h"

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Support format specifiers.
	- [ ] Compile time check string format.
*/

inline static constexpr const char *FORMAT_DIGITS_LOWERCASE = "0123456789abcdef";
inline static constexpr const char *FORMAT_DIGITS_UPPERCASE = "0123456789ABCDEF";

template <typename ...TArgs>
inline static String
format2(const char *fmt, TArgs &&...args);

template <typename T>
requires (std::is_integral_v<T> && !std::is_floating_point_v<T>)
inline static String
format2(T data, u8 base = 10, bool uppercase = false)
{
	const char *digits = uppercase ? FORMAT_DIGITS_UPPERCASE : FORMAT_DIGITS_LOWERCASE;

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
format2(T data)
{
	String buffer = string_init(memory::temp_allocator());

	if (data < 0)
	{
		string_append(buffer, '-');
		data = -data;
	}

	u64 integer = (u64)data;
	f64 fraction = data - integer;
	string_append(buffer, format2((u64)integer));
	string_append(buffer, '.');

	//
	// NOTE:
	// Default precision is 6.
	//
	for (u64 i = 0; i < 6; ++i)
	{
		fraction *= 10;
		integer = (u64)fraction;
		string_append(buffer, format2(integer));
		fraction = fraction - integer;
	}

	while (string_ends_with(buffer, '0'))
		string_remove_last(buffer);

	if (string_ends_with(buffer, '.'))
		string_remove_last(buffer);

	return buffer;
}

inline static String
format2(bool data)
{
	String buffer = string_init(memory::temp_allocator());
	string_append(buffer, data ? "true" : "false");
	return buffer;
}

inline static String
format2(char data)
{
	String buffer = string_init(memory::temp_allocator());
	string_append(buffer, data);
	return buffer;
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<std::remove_cvref_t<T>, char *>)
inline static String
format2(const T &data)
{
	return format2((uptr)data, 16, true);
}

template <typename T, u64 N>
inline static String
format2(const T (&data)[N])
{
	String buffer = string_init(memory::temp_allocator());

	if constexpr (std::is_same_v<T, char>)
	{
		for (u64 i = 0; i < N; ++i)
			string_append(buffer, data[i]);
	}
	else
	{
		string_append(buffer, format2("[{}] {{ ", N));
		for (u64 i = 0; i < N; ++i)
		{
			if (i != 0)
				string_append(buffer, ", ");
			string_append(buffer, format2(data[i]));
		}
		string_append(buffer, " }}");
	}

	return buffer;
}

template <typename ...TArgs>
inline static String
format2(const char *fmt, TArgs &&...args)
{
	// 1. Validate that the format string is correct, and loop over all the replacement fields and match them against the arguments.
	// 2. Format each argument into the corresponding replacement field.
	// 3. Append the result string into the output buffer.

	String fmt_string = string_literal(fmt);
	if (string_is_empty(fmt_string))
		return string_literal("");

	String buffer = string_init(memory::temp_allocator());

	u64 replacement_field_count = 0;
	for (u64 i = 0; i < fmt_string.count; ++i)
	{
		if (fmt_string[i] == '{')
		{
			if (i + 1 < fmt_string.count)
			{
				switch (fmt_string[i + 1])
				{
					case '{':
					{
						string_append(buffer, '{');
						++i;
						break;
					}
					case '}':
					{
						++i;

						i32 index = 0;
						([&](const auto &arg)
						{
							if (index == replacement_field_count)
							{
								string_append(buffer, format2(arg));
							}
							++index;
						}(args), ...);

						++replacement_field_count;
						break;
					}
				}
			}
			else
			{
				validate(false, "[FORMAT]: '{' must have a matching '{' or '}'.");
			}
		}
		else if (fmt_string[i] == '}')
		{
			if (i + 1 < fmt_string.count && fmt_string[i + 1] == '}')
			{
				string_append(buffer, '}');
				++i;
			}
			else
			{
				validate(false, "[FORMAT]: '}' must have a matching '}'.");
			}
		}
		else
		{
			string_append(buffer, fmt_string[i]);
		}
	}

	validate(replacement_field_count == sizeof...(args), "[FORMAT]: Replacement field count does not match argument count.");

	return buffer;
}