#pragma once

#include "core/defines.h"

#include <functional>
#include <type_traits>

/*
	TODO:
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Support format specifiers.
	- [ ] Cleanup, simplify and collapse parse and flush functions into one.
*/

#define FORMAT(T) \
void              \
format(T data);

struct Formatter
{
	struct Formatter_Context *ctx;

	const char *buffer;

	Formatter();
	~Formatter();

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
	parse_begin(const char *fmt, u64 arg_count);

	void
	parse(std::function<void()> &&callback);

	void
	flush();
};

template <typename ...TArgs>
inline static void
formatter_format(Formatter &self, const char *fmt, const TArgs &...args)
{
	self.parse_begin(fmt, sizeof...(args));
	if constexpr (sizeof...(args))
		(self.parse([&]() { format(self, args); }), ...);
	else
		self.parse([&]() { });
	self.flush();
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
requires (std::is_pointer_v<T>)
inline static void
format(Formatter &self, const T data)
{
	if constexpr (std::is_same_v<T, char *> || std::is_same_v<T, const char *>)
		self.format((const char *)data);
	else
		self.format((const void *)data);
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char>)
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