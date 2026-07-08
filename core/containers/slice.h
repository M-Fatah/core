#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/containers/array.h"
#include "core/containers/stack_array.h"

#include <type_traits>
#include <initializer_list>

template <typename T>
struct Slice
{
	T *data;
	U64 count;

	Slice()
	{
		Slice &self = *this;
		self.data = nullptr;
		self.count = 0;
	}

	Slice(T &value)
	{
		Slice &self = *this;
		self.data = &value;
		self.count = 1;
	}

	Slice(T *data, U64 count)
	{
		Slice &self = *this;
		self.data = data;
		self.count = count;
	}

	template <U64 N>
	requires (!is_same_v<T, const char>)
	Slice(T (&array)[N])
	{
		Slice &self = *this;
		self.data = array;
		self.count = N;
	}

	template <typename R>
	requires (is_same_v<T, const R>)
	Slice(std::initializer_list<R> values)
	{
		Slice &self = *this;
		self.data = values.begin();
		self.count = (U64)values.size();
	}

	template <typename R>
	requires (is_same_v<T, R> || is_same_v<T, const R>)
	Slice(Slice<R> other)
	{
		Slice &self = *this;
		self.data = other.data;
		self.count = other.count;
	}

	inline T &
	operator[](U64 index)
	{
		validate(index < count, "[SLICE]: Access out of range.");
		return data[index];
	}

	inline const T &
	operator[](U64 index) const
	{
		validate(index < count, "[SLICE]: Access out of range.");
		return data[index];
	}
};

template <typename T>
requires (!std::is_array_v<T>)
inline static Slice<T>
slice_from(T &value)
{
	return Slice<T>(value);
}

template <typename T>
inline static Slice<T>
slice_from(T *data, U64 count)
{
	return Slice<T>(data, count);
}

template <typename T>
inline static Slice<T>
slice_from(T *begin, T *end)
{
	return Slice<T>(begin, (U64)(end - begin));
}

template <typename T, U64 N>
requires (!is_same_v<T, const char>)
inline static Slice<T>
slice_from(T (&array)[N])
{
	return Slice<T>(array, N);
}

template <typename T>
inline static Slice<const T>
slice_from(std::initializer_list<T> values)
{
	return Slice<const T>(values.begin(), (U64)values.size());
}

template <typename T>
inline static Slice<T>
slice_from(Array<T> &array)
{
	return Slice<T>(array.data, array.count);
}

template <typename T>
inline static Slice<const T>
slice_from(const Array<T> &array)
{
	return Slice<const T>(array.data, array.count);
}

template <typename T, U64 N>
inline static Slice<T>
slice_from(Stack_Array<T, N> &array)
{
	return Slice<T>(array.data, array.count);
}

template <typename T, U64 N>
inline static Slice<const T>
slice_from(const Stack_Array<T, N> &array)
{
	return Slice<const T>(array.data, array.count);
}

inline static Slice<const char>
slice_from(const char *string)
{
	if (string == nullptr)
		return Slice<const char>{};

	U64 count = 0;
	while (string[count] != '\0')
		++count;
	return Slice<const char>(string, count);
}

template <typename T>
inline static bool
slice_is_empty(const Slice<T> &self)
{
	return self.count == 0;
}

template <typename T>
inline static T &
slice_front(Slice<T> &self)
{
	validate(self.count > 0, "[SLICE]: Count is 0.");
	return self[0];
}

template <typename T>
inline static const T &
slice_front(const Slice<T> &self)
{
	validate(self.count > 0, "[SLICE]: Count is 0.");
	return self[0];
}

template <typename T>
inline static T &
slice_back(Slice<T> &self)
{
	validate(self.count > 0, "[SLICE]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static const T &
slice_back(const Slice<T> &self)
{
	validate(self.count > 0, "[SLICE]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static T *
begin(Slice<T> &self)
{
	return self.data;
}

template <typename T>
inline static const T *
begin(const Slice<T> &self)
{
	return self.data;
}

template <typename T>
inline static T *
end(Slice<T> &self)
{
	return self.data + self.count;
}

template <typename T>
inline static const T *
end(const Slice<T> &self)
{
	return self.data + self.count;
}