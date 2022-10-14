#pragma once

#include <core/defines.h>

#include <stdio.h>
#include <string.h>
#include <type_traits>

/*
	TODO:
	- [x] Overload 'format' function like serializer instead?
	- [x] Remove libfmt dependency.
	- [x] Collapse the two 'formatter_format' functions.
	- [x] Collapse the two 'formatter_parse' functions.
	- [x] Add unittests.
	- [x] Add support for arrays.
	- [x] Template types names differ between Windows/Linux.
	- [x] Check for matching count of replacement_characters and argument count.
	- [x] Move implementation to .cpp file.
	- [ ] Remove the 32KB buffer size restriction.
	- [ ] Simplify and optimize.
	- [ ] Do not rely on ::snprintf and implement our own formatting.
	- [ ] Pointer formatting differ between Windows/Linux.
*/

#define FORMAT(T) \
void              \
format(T data);

struct Formatter
{
	char buffer[32 * 1024];
	u64 index;
	u64 replacement_character_count;
	u64 depth;

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
	FORMAT(char *)
	FORMAT(void *)

	void
	clear();
};

#undef FORMAT

template <typename T>
inline static void
format(Formatter &, T)
{
	static_assert(sizeof(T) == 0, "There is no `void format(Formatter &, T)` function overload defined for this type.");
}

#define FORMAT(T)               \
inline static void              \
format(Formatter &self, T data) \
{                               \
	self.format(data);          \
}                               \

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
FORMAT(char *)
FORMAT(void *)

#undef FORMAT

template <typename ...TArgs>
inline static void
format(Formatter &self, const char *fmt, const TArgs &...args)
{
	u64 fmt_count = ::strlen(fmt);
	if (fmt_count == 0)
		return;

	u64 start = 0;
	([&] (const auto &arg)
	{
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
				if (self.depth == 0)
					self.replacement_character_count++;
				start = i + 1;
				self.depth++;
				format(self, arg);
				self.depth--;
				return;
			}

			if (fmt[i] == '{' || fmt[i] == '}')
			{
				continue;
			}

			self.buffer[self.index++] = fmt[i];
		}
	} (args), ...);

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
			// TODO: For now we will eat the replacement characters and do nothing.
			if (self.depth == 0)
			{
				i++;
				self.replacement_character_count++;
			}
			else
			{
				self.buffer[self.index++] = '{';
				self.buffer[self.index++] = '}';

			}
			continue;
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
	self.clear();
}

// TODO: Can we move this to cpp file?
template <typename T>
concept Array_Type = !std::is_same_v<T, char>;

template <Array_Type T, u64 N>
inline static void
format(Formatter &self, const T(&data)[N])
{
	u64 count = N;
	format(self, "[{}] {{ ", count);
	for (u64 i = 0; i < count; ++i)
	{
		if (i != 0)
			format(self, ", ");
		format(self, data[i]);
	}
	format(self, " }}");
}