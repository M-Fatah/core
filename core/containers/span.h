#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/containers/array.h"
#include "core/containers/stack_array.h"

#include <initializer_list>
#include <type_traits>

/*
TODO:
- [ ] Consider naming _init to _from, since we don't allocate memory.
*/

template <typename T>
struct Span
{
	T *data;
	U64 count;

	// Default — empty span.
	Span() : data(nullptr), count(0) {}

	// Trivially copyable / movable — explicit so our converting constructors
	// below don't inhibit the compiler-synthesized copy.
	Span(const Span &) = default;
	Span(Span &&) = default;
	Span &operator=(const Span &) = default;
	Span &operator=(Span &&) = default;

	// Raw pointer + count.
	Span(T *data_, U64 count_) : data(data_), count(count_) {}

	// Half-open [begin, end) range.
	Span(T *begin_, T *end_) : data(begin_), count(U64(end_ - begin_)) {}

	// C-style array.
	template <U64 N>
	Span(T (&array)[N]) : data(array), count(N) {}

	// From the engine's containers.
	Span(Array<T> &array) : data(array.data), count(array.count) {}

	template <U64 N>
	Span(Stack_Array<T, N> &array) : data(array.data), count(array.count) {}

	// Single element — span of size 1 over the given value's storage. The caller
	// must keep the referenced value alive for the span's lifetime.
	Span(T &value) : data(&value), count(1) {}

	// Convert Span<T> to Span<const T> (the mutable → const view). Only enabled
	// when T is `const U` so this doesn't shadow the copy constructor.
	Span(const Span<std::remove_const_t<T>> &other) requires std::is_const_v<T>
		: data(other.data), count(other.count) {}

	inline T &
	operator[](U64 index)
	{
		validate(index < count, "[SPAN]: Access out of range.");
		return data[index];
	}

	inline const T &
	operator[](U64 index) const
	{
		validate(index < count, "[SPAN]: Access out of range.");
		return data[index];
	}
};

// Deduction guides so `Span{data, count}` and friends pick up T without a user annotation.
template <typename T> Span(T *, U64) -> Span<T>;
template <typename T> Span(T *, T *) -> Span<T>;
template <typename T, U64 N> Span(T (&)[N]) -> Span<T>;
template <typename T> Span(Array<T> &) -> Span<T>;
template <typename T, U64 N> Span(Stack_Array<T, N> &) -> Span<T>;
template <typename T> Span(T &) -> Span<T>;

// ---- span_init helpers (retained for existing call sites) ------------------

template <typename T>
inline static Span<T>
span_init(T *data, U64 count)
{
	return Span<T>(data, count);
}

template <typename T>
inline static Span<T>
span_init(T *begin, T *end)
{
	return Span<T>(begin, end);
}

template <typename T, U64 N>
inline static Span<T>
span_init(T (&array)[N])
{
	return Span<T>(array);
}

template <typename T>
inline static Span<const T>
span_init(std::initializer_list<T> list)
{
	return Span<const T>(list.begin(), U64(list.size()));
}

template <typename T>
inline static Span<T>
span_init(Array<T> &array)
{
	return Span<T>(array);
}

template <typename T, U64 N>
inline static Span<T>
span_init(Stack_Array<T, N> &array)
{
	return Span<T>(array);
}

template <typename T>
inline static Span<T>
span_init(T &value)
{
	return Span<T>(value);
}

inline static Span<const char>
span_init(const char *string)
{
	constexpr auto c_string_length = [](const char *str) -> U64 {
		U64 length = 0;
		while (str[length] != '\0')
			++length;
		return length;
	};
	return Span<const char>(string, c_string_length(string));
}

// ---- Accessors / iteration -------------------------------------------------

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