#pragma once

#include <core/defines.h>

#include <type_traits>

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Add format specifiers.
	- [ ] Remove the 32KB buffer size restriction.
	- [ ] Try and workaround moving implementation to cpp file.
	- [ ] Use template Formatter<> struct?
	- [ ] Make formatter_format and format function return const char *?
	- [ ] Move concepts to the top of the header files?
*/

#define FORMAT(T) \
void              \
format(T data);

static constexpr u64 FORMATTER_BUFFER_MAX_SIZE = 32 * 1024;

struct Formatter
{
	char buffer[FORMATTER_BUFFER_MAX_SIZE];
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
	FORMAT(const char *)
	FORMAT(const void *)

	void
	clear();
};

template <typename ...TArgs>
inline static void
formatter_format(Formatter &self, const char *fmt, const TArgs &...args)
{
	u64 fmt_count = 0;
	const char *fmt_ptr = fmt;
	while(*fmt_ptr)
	{
		++fmt_count;
		++fmt_ptr;
	}

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
				if (self.index < FORMATTER_BUFFER_MAX_SIZE)
					self.buffer[self.index++] = '{';
				continue;
			}

			if (fmt[i] == '}' && fmt[i + 1] == '}')
			{
				i++;
				if (self.index < FORMATTER_BUFFER_MAX_SIZE)
					self.buffer[self.index++] = '}';
				continue;
			}

			if (fmt[i] == '{' && fmt[i + 1] == '}')
			{
				i++;
				if (self.depth == 0)
					self.replacement_character_count++;
				start = i + 1;
				++self.depth;
				format(self, arg);
				--self.depth;
				return;
			}

			if (fmt[i] == '{' || fmt[i] == '}')
			{
				continue;
			}

			if (self.index < FORMATTER_BUFFER_MAX_SIZE)
				self.buffer[self.index++] = fmt[i];
		}
	} (args), ...);

	for (u64 i = start; i < fmt_count; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '{')
		{
			i++;
			if (self.index < FORMATTER_BUFFER_MAX_SIZE)
				self.buffer[self.index++] = '{';
			continue;
		}

		if (fmt[i] == '}' && fmt[i + 1] == '}')
		{
			i++;
			if (self.index < FORMATTER_BUFFER_MAX_SIZE)
				self.buffer[self.index++] = '}';
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			if (self.depth == 0)
			{
				//
				// NOTE:
				// The replacement character count is larger than the number of passed arguments,
				//    at this point we just eat them.
				//
				i++;
				self.replacement_character_count++;
			}
			else
			{
				//
				// NOTE:
				// The user passed "{}" replacement character as an argument, we just append it,
				//    for e.x. formatter_format(formatter, "{}", "{}"); => "{}".
				//
				if (self.index < FORMATTER_BUFFER_MAX_SIZE - 1)
				{
					self.buffer[self.index++] = '{';
					self.buffer[self.index++] = '}';
				}
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
			{
				if (self.index < FORMATTER_BUFFER_MAX_SIZE)
					self.buffer[self.index++] = fmt[i];
			}
		}
		else
		{
			if (self.index < FORMATTER_BUFFER_MAX_SIZE)
				self.buffer[self.index++] = fmt[i];
		}
	}

	if (self.index < FORMATTER_BUFFER_MAX_SIZE)
		self.buffer[self.index] = '\0';
	else
		self.buffer[FORMATTER_BUFFER_MAX_SIZE - 1] = '\0';
}

inline static void
formatter_clear(Formatter &self)
{
	self.clear();
}

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

#undef FORMAT

template <typename ...TArgs>
inline static void
format(Formatter &self, const char *fmt, const TArgs &...args)
{
	formatter_format(self, fmt, args...);
}

template <typename T>
concept Pointer_Type = std::is_pointer_v<T>;

template <Pointer_Type T>
inline static void
format(Formatter &self, const T data)
{
	if constexpr (std::is_same_v<T, char *> || std::is_same_v<T, const char *>)
		self.format((const char *)data);
	else
		self.format((const void *)data);
}

template <typename T>
concept Array_Type = !std::is_same_v<T, char>;

template <Array_Type T, u64 N>
inline static void
format(Formatter &self, const T (&data)[N])
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