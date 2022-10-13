#pragma once

#include <core/defines.h>

#include <stdio.h>
#include <string.h>
#include <typeinfo>
#include <type_traits>

/*
	TODO:
	- [x] Overload 'format' function like serializer instead?
	- [x] Remove libfmt dependency.
	- [x] Collapse the two 'formatter_format' functions.
	- [x] Add unittests.
	- [x] Add support for arrays.
	- [ ] Pointer formatting differ between Windows/Linux.
	- [ ] Template types names differ between Windows/Linux.
	- [ ] Collapse the two 'formatter_parse' functions.
	- [ ] Remove the 32KB buffer size restriction.
	- [ ] Move implementation to .cpp file.
	- [ ] Check for matching count of replacement_characters and argument count.
	- [ ] Simplify and optimize.
	- [ ] Do not rely on ::snprintf and implement our own formatting.
*/

struct Formatter
{
	char buffer[32 * 1024];
	u64 index;
};

template <typename T>
inline static void
format(Formatter &, const T &)
{
	static_assert(sizeof(T) == 0, "There is no `void format(Formatter &, const T &)` function overload defined for this type.");
}

template<typename T, size_t N>
constexpr size_t countof(const T(&)[N]) noexcept
{
	return N;
}

template <typename T>
using array_element_type = std::decay_t<decltype(std::declval<T&>()[0])>;

template <typename T>
inline static void
formatter_format(Formatter &self, const T &value)
{
	if constexpr (std::is_pointer_v<T>)
	{
		if constexpr (std::is_same_v<T, char *>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
		else if constexpr (std::is_same_v<T, const char *>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
		else if constexpr (std::is_same_v<T, const char * const>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
		else
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%p", (void *)value);
	}
	else if constexpr (std::is_same_v<T, char>)
	{
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%c", value);
	}
	else if constexpr (std::is_same_v<T, bool>)
	{
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value ? "true" : "false");
	}
	else if constexpr (std::is_floating_point_v<T>)
	{
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%g", value);
	}
	else if constexpr (std::is_integral_v<T>)
	{
		if constexpr (std::is_same_v<T, long long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%lld", value);
		else if constexpr (std::is_same_v<T, long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%ld", value);
		else if constexpr (std::is_same_v<T, unsigned long long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%lld", value);
		else if constexpr (std::is_same_v<T, unsigned long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%ld", value);
		else
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%d", value);
	}
	else if constexpr (std::is_array_v<T>)
	{
		if constexpr (std::is_same_v<array_element_type<T>, char>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
	}
	else
	{
		static_assert(sizeof(T) == 0, "There is no `void formatter_format(Formatter &, const T &)` function overload defined for this type.");
	}
}

#define FORMAT(T)                      \
inline static void                     \
format(Formatter &self, const T *data) \
{                                      \
	formatter_format(self, data);      \
}                                      \
                                       \
inline static void                     \
format(Formatter &self, T *data)       \
{                                      \
	formatter_format(self, data);      \
}                                      \

FORMAT(i8)
FORMAT(i16)
FORMAT(i32)
FORMAT(i64)
FORMAT(u8)
FORMAT(u16)
FORMAT(u32)
FORMAT(u64)
FORMAT(f32)
FORMAT(f64)
FORMAT(bool)
FORMAT(char)
FORMAT(void)

#undef FORMAT

#define FORMAT(T)                      \
inline static void                     \
format(Formatter &self, const T &data) \
{                                      \
	formatter_format(self, data);      \
}

FORMAT(i8)
FORMAT(i16)
FORMAT(i32)
FORMAT(i64)
FORMAT(u8)
FORMAT(u16)
FORMAT(u32)
FORMAT(u64)
FORMAT(f32)
FORMAT(f64)
FORMAT(bool)
FORMAT(char)

#undef FORMAT

template <typename T>
inline static void
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
			format(self, t);
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
inline static void
formatter_parse(Formatter &self, const char *fmt, const TArgs &...args)
{
	u64 fmt_count = ::strlen(fmt);
	if (fmt_count == 0)
		return;

	u64 start = 0;
	(formatter_parse(self, fmt, start, args), ...);

	for (u64 i = start; i < fmt_count; ++i)
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
			// TODO:
		}

		if (fmt[i] == '{' || fmt[i] == '}')
		{
			continue;
		}

		if (i == (fmt_count - 1))
		{
			if (fmt[i] != '{' && fmt[i] != '}')
				self.buffer[self.index++] = fmt[i];
		}
		else
		{
			self.buffer[self.index++] = fmt[i];
		}
	}

	self.buffer[self.index] = '\0';
}

inline static void
formatter_clear(Formatter &self)
{
	self.index = 0;
}

template <typename T, u64 N>
inline static void
format(Formatter &self, const T(&data)[N])
{
	u64 count = N;
	formatter_parse(self, "[{}] {{ ", N);
	for (u64 i = 0; i < count; ++i)
	{
		if (i != 0)
			formatter_format(self, ", ");
		format(self, data[i]);
	}
	formatter_format(self, " }");
}