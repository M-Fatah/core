#pragma once

#include "core/defines.h"

#include <functional>
#include <type_traits>

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Support format specifiers.
	- [ ] Make formatter_parse_begin() compile time?
	- [ ] Cleanup, simplify and collapse parse functions into one.
*/

struct Formatter;

Formatter *
formatter();

bool
formatter_parse_begin(Formatter *self, const char *fmt, u64 arg_count);

void
formatter_parse_next(Formatter *self, std::function<void()> &&callback);

const char *
formatter_parse_end(Formatter *self);

template <typename T>
inline static const char *
format(Formatter *, T)
{
	static_assert(sizeof(T) == 0, "There is no `const char * format(Formatter *, T)` function overload defined for this type.");
	return "";
}

#define FORMAT(T)                \
const char *                     \
format(Formatter *self, T data);

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
FORMAT(const void *)

#undef FORMAT

template <typename ...TArgs>
inline static const char *
format(Formatter *self, const char *fmt, const TArgs &...args)
{
	if (formatter_parse_begin(self, fmt, sizeof...(args)))
	{
		(formatter_parse_next(self, [&]() { format(self, args); }), ...);
		return formatter_parse_end(self);
	}
	return "";
}

template <typename T>
requires (std::is_pointer_v<T>)
inline static const char *
format(Formatter *self, const T data)
{
	if constexpr (std::is_same_v<T, char *>)
		return format(self, (const char *)data);
	else
		return format(self, (const void *)data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char>)
inline static const char *
format(Formatter *self, const T (&data)[N])
{
	u64 count = N;
	format(self, "[{}] {{ ", count);
	for (u64 i = 0; i < count; ++i)
	{
		if (i != 0)
			format(self, ", ");
		format(self, data[i]);
	}
	return format(self, " }}");
}

template <typename ...TArgs>
inline static const char *
format(const char *fmt, const TArgs &...args)
{
	return format(formatter(), fmt, args...);
}