#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/containers/array.h"
#include "core/containers/stack_array.h"

#include <initializer_list>

/*
TODO:
- [ ] Consider naming _init to _from, since we don't allocate memory.
*/

template <typename T>
struct Span
{
	T *data;
	u64 count;

	inline T &
	operator[](u64 index)
	{
		validate(index < count, "[SPAN]: Access out of range.");
		return data[index];
	}

	inline const T &
	operator[](u64 index) const
	{
		validate(index < count, "[SPAN]: Access out of range.");
		return data[index];
	}
};

template <typename T>
inline static Span<T>
span_init(T *data, u64 count)
{
	return Span<T> {
		.data = data,
		.count = count,
	};
}

template <typename T>
inline static Span<T>
span_init(T *begin, T *end)
{
	return span_init(begin, u64(end - begin));
}

template <typename T, u64 N>
inline static Span<T>
span_init(T (&array)[N])
{
	return span_init(array, N);
}

template <typename T>
inline static Span<const T>
span_init(std::initializer_list<T> list)
{
	return Span<const T>{ .data = list.begin(), .count = list.size() };
}

template <typename T>
inline static Span<T>
span_init(Array<T> &array)
{
	return span_init(array.data, array.count);
}

template <typename T, u64 N>
inline static Span<T>
span_init(Stack_Array<T, N> &array)
{
	return span_init(array.data, array.count);
}

inline static Span<const char>
span_init(const char *string)
{
	constexpr auto c_string_length = [](const char *str) -> u64 {
		u64 length = 0;
		while (str[length] != '\0')
			++length;
		return length;
	};
	return span_init(string, c_string_length(string));
}

template <typename T>
inline static bool
span_is_empty(const Span<T> &self)
{
	return self.count == 0;
}

template <typename T>
inline static T &
span_first(Span<T> &self)
{
	validate(self.count > 0, "[SPAN]: Count is 0.");
	return self[0];
}

template <typename T>
inline static const T &
span_first(const Span<T> &self)
{
	validate(self.count > 0, "[SPAN]: Count is 0.");
	return self[0];
}

template <typename T>
inline static T &
span_last(Span<T> &self)
{
	validate(self.count > 0, "[SPAN]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static const T &
span_last(const Span<T> &self)
{
	validate(self.count > 0, "[SPAN]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static T *
begin(Span<T> &self)
{
	return self.data;
}

template <typename T>
inline static const T *
begin(const Span<T> &self)
{
	return self.data;
}

template <typename T>
inline static T *
end(Span<T> &self)
{
	return self.data + self.count;
}

template <typename T>
inline static const T *
end(const Span<T> &self)
{
	return self.data + self.count;
}