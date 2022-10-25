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
	parse_begin(const char *fmt);

	void
	parse(const char *fmt, u64 &start, u64 arg_count, std::function<void()> &&callback);

	void
	flush(const char *fmt, u64 start);

	void
	clear();
};

template <typename ...TArgs>
inline static void
formatter_format(Formatter &self, const char *fmt, const TArgs &...args)
{
	u64 start = 0;
	self.parse_begin(fmt);
	if constexpr (sizeof...(args) > 0)
		(self.parse(fmt, start, sizeof...(args), [&]() { format(self, args); }), ...);
	else
		self.parse(fmt, start, 0, []() { });
	self.flush(fmt, start);
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