#pragma once

#include <core/defines.h>

#include <functional>
#include <type_traits>

/*
	TODO:
	- [x] Try and workaround moving implementation to cpp file.
	- [ ] Implement 100% correct floating point formatting.
	- [ ] Add format specifiers.
	- [ ] Remove the 32KB buffer size restriction.
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

	// TODO:
	void
	parse(const char *fmt, u64 &start, std::function<void()> &&function);

	// TODO:
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
	(self.parse(fmt, start, [&]() { format(self, args); }), ...);
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