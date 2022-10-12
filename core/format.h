#pragma once

#include <core/defines.h>

#include <string.h>
#include <typeinfo>
#include <type_traits>

struct Formatter
{
	char buffer[32 * 1024];
	u64 index;
};

// TODO: Collapse.
template <typename T>
inline static void
formatter_format(char (&buffer)[4096], u64 &index, const T *value)
{
	if constexpr (std::is_same_v<T *, char *>)
		index += ::snprintf(buffer + index, sizeof(buffer), "%s", value);
	else if constexpr (std::is_same_v<const T *, const char *>)
		index += ::snprintf(buffer + index, sizeof(buffer), "%s", value);
	else
		index += ::snprintf(buffer + index, sizeof(buffer), "%p", value);
}

template <typename T>
inline static void
formatter_format(char (&buffer)[4096], u64 &index, const T &value)
{
	if constexpr (std::is_pointer_v<T>)
		index += ::snprintf(buffer + index, sizeof(buffer), "%p", value);
	else if constexpr (std::is_same_v<T, char>)
		index += ::snprintf(buffer + index, sizeof(buffer), "%c", value);
	else if constexpr (std::is_same_v<T, bool>)
		index += ::snprintf(buffer + index, sizeof(buffer), "%s", value ? "true" : "false");
	else if constexpr (std::is_floating_point_v<T>)
		index += ::snprintf(buffer + index, sizeof(buffer), "%g", value);
	else if constexpr (std::is_integral_v<T>)
	{
		if constexpr (sizeof(T) == 8)
			index += ::snprintf(buffer + index, sizeof(buffer), "%lld", value);
		else
			index += ::snprintf(buffer + index, sizeof(buffer), "%ld", value);
	}
}

template <typename T>
void
format_print(const char *fmt, const T &t)
{
	// Formatter formatter = {};

	u64 fmt_count = ::strlen(fmt);
	if (fmt_count == 0)
		return;

	char buffer[4096];
	u64 index = 0;

	u64 replacement_character_count = 0;
	for (u64 i = 0; i < fmt_count - 1; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '{')
		{
			i++;
			buffer[index++] = '{';
			continue;
		}

		if (fmt[i] == '}' && fmt[i + 1] == '}')
		{
			i++;
			buffer[index++] = '}';
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			i++;
			replacement_character_count++;
			formatter_format(buffer, index, t);
			continue;
		}

		if (fmt[i] == '{' || fmt[i] == '}')
		{
			continue;
		}

		buffer[index++] = fmt[i];
	}

	if (fmt[fmt_count - 1] != '{' && fmt[fmt_count - 1] != '}')
		buffer[index++] = fmt[fmt_count - 1];

	buffer[index] = '\0';

	// ::printf("Replacement character count: %lld\n", replacement_character_count);
	::printf("Buffer content: [%lld] %s\n", index, buffer);
}