#pragma once

#include <core/defines.h>

#include <string.h>
#include <typeinfo>
#include <type_traits>

/*
	TODO:
	- [ ] Remove the 32KB buffer size restriction.
	- [ ] Move implementation to .cpp file.
	- [ ] Overload 'format' function like serializer instead?
	- [ ] Collapse the two 'formatter_format' functions.
	- [ ] Check for matching count of replacement_characters and argument count.
	- [ ] Simplify and optimize.
*/

struct Formatter
{
	char buffer[32 * 1024];
	u64 index;
};

template <typename T>
inline static void
formatter_format(Formatter &self, const T *value)
{
	if constexpr (std::is_same_v<T *, char *>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
	else if constexpr (std::is_same_v<const T *, const char *>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
	else
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%p", value);
}

template <typename T>
inline static void
formatter_format(Formatter &self, const T &value)
{
	if constexpr (std::is_pointer_v<T>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%p", value);
	else if constexpr (std::is_same_v<T, char>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%c", value);
	else if constexpr (std::is_same_v<T, bool>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value ? "true" : "false");
	else if constexpr (std::is_floating_point_v<T>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%g", value);
	else if constexpr (std::is_integral_v<T>)
	{
		if constexpr (sizeof(T) == 8)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%lld", value);
		else
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%ld", value);
	}
}

template <typename T>
void
formatter_parse(Formatter &self, const char *fmt, u64 &start, const T &t)
{
	u64 fmt_count = ::strlen(fmt);
	if (fmt_count == 0)
		return;

	u64 replacement_character_count = 0;
	for (u64 i = start; i < fmt_count - 1; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '{')
		{
			i++;
			self.buffer[self.index++] = '{';
			continue;
		}

		if (fmt[i] == '}' && fmt[i + 1] == '}')
		{
			i++;
			self.buffer[self.index++] = '}';
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			i++;
			replacement_character_count++;
			start = i + 1;
			formatter_format(self, t);
			return;
		}

		if (fmt[i] == '{' || fmt[i] == '}')
		{
			continue;
		}

		self.buffer[self.index++] = fmt[i];
	}
}

template <typename ...TArgs>
void
formatter_print(const char *fmt, const TArgs &...args)
{
	u64 fmt_count = ::strlen(fmt);
	if (fmt_count == 0)
		return;

	Formatter self = {};

	u64 start = 0;
	(formatter_parse(self, fmt, start, args), ...);

	if (fmt[fmt_count - 1] != '{' && fmt[fmt_count - 1] != '}')
		self.buffer[self.index++] = fmt[fmt_count - 1];

	self.buffer[self.index] = '\0';

	// TODO: Remove.
	// ::printf("Replacement character count: %lld\n", replacement_character_count);
	::printf("Buffer content: [%lld] %s\n", self.index, self.buffer);
}