#pragma once

#include "core/defines.h"
#include "core/hash.h"
#include "core/validate.h"
#include "core/containers/array.h"
#include "core/containers/stack_array.h"

#include <type_traits>
#include <initializer_list>

template <typename T>
struct View
{
	T *data;
	U64 count;

	View()
	{
		View &self = *this;
		self.data = nullptr;
		self.count = 0;
	}

	View(T &value)
	{
		View &self = *this;
		self.data = &value;
		self.count = 1;
	}

	View(T *data, U64 count)
	{
		View &self = *this;
		self.data = data;
		self.count = count;
	}

	template <U64 N>
	requires (!is_same_v<T, const char>)
	View(T (&array)[N])
	{
		View &self = *this;
		self.data = array;
		self.count = N;
	}

	template <typename R>
	requires (is_same_v<T, const R>)
	View(std::initializer_list<R> values)
	{
		View &self = *this;
		self.data = values.begin();
		self.count = (U64)values.size();
	}

	template <typename R>
	requires (is_same_v<T, R> || is_same_v<T, const R>)
	View(View<R> other)
	{
		View &self = *this;
		self.data = other.data;
		self.count = other.count;
	}

	inline T &
	operator[](U64 index)
	{
		validate(index < count, "[VIEW]: Access out of range.");
		return data[index];
	}

	inline const T &
	operator[](U64 index) const
	{
		validate(index < count, "[VIEW]: Access out of range.");
		return data[index];
	}

	inline bool
	operator==(const View &other) const
	requires (is_same_v<T, const char>)
	{
		const View &self = *this;
		if (self.count != other.count)
			return false;

		for (U64 i = 0; i < self.count; ++i)
			if (self.data[i] != other.data[i])
				return false;
		return true;
	}

	inline bool
	operator!=(const View &other) const
	requires (is_same_v<T, const char>)
	{
		const View &self = *this;
		return !(self == other);
	}

	inline bool
	operator==(const char *other) const
	requires (is_same_v<T, const char>)
	{
		const View &self = *this;
		if (other == nullptr)
			return self.count == 0;

		for (U64 i = 0; i < self.count; ++i)
			if (other[i] == '\0' || self.data[i] != other[i])
				return false;
		return other[self.count] == '\0';
	}

	inline bool
	operator!=(const char *other) const
	requires (is_same_v<T, const char>)
	{
		const View &self = *this;
		return !(self == other);
	}

	friend inline bool
	operator==(const char *self, const View &other)
	requires (is_same_v<T, const char>)
	{
		return other == self;
	}

	friend inline bool
	operator!=(const char *self, const View &other)
	requires (is_same_v<T, const char>)
	{
		return other != self;
	}
};

using String_View = View<const char>;

template <typename T>
requires (!std::is_array_v<T>)
inline static View<T>
view_from(T &value)
{
	return View<T>(value);
}

template <typename T>
inline static View<T>
view_from(T *data, U64 count)
{
	return View<T>(data, count);
}

template <typename T>
inline static View<T>
view_from(T *begin, T *end)
{
	return View<T>(begin, (U64)(end - begin));
}

template <typename T, U64 N>
requires (!is_same_v<T, const char>)
inline static View<T>
view_from(T (&array)[N])
{
	return View<T>(array, N);
}

template <typename T>
inline static View<const T>
view_from(std::initializer_list<T> values)
{
	return View<const T>(values.begin(), (U64)values.size());
}

template <typename T>
inline static View<T>
view_from(Array<T> &array)
{
	return View<T>(array.data, array.count);
}

template <typename T>
inline static View<const T>
view_from(const Array<T> &array)
{
	return View<const T>(array.data, array.count);
}

template <typename T, U64 N>
inline static View<T>
view_from(Stack_Array<T, N> &array)
{
	return View<T>(array.data, array.count);
}

template <typename T, U64 N>
inline static View<const T>
view_from(const Stack_Array<T, N> &array)
{
	return View<const T>(array.data, array.count);
}

inline static String_View
view_from(const char *string)
{
	if (string == nullptr)
		return String_View{};

	U64 count = 0;
	while (string[count] != '\0')
		++count;

	return String_View(string, count);
}

template <typename T>
inline static bool
view_is_empty(const View<T> &self)
{
	return self.count == 0;
}

template <typename T>
inline static T &
view_front(View<T> &self)
{
	validate(self.count > 0, "[VIEW]: Count is 0.");
	return self[0];
}

template <typename T>
inline static const T &
view_front(const View<T> &self)
{
	validate(self.count > 0, "[VIEW]: Count is 0.");
	return self[0];
}

template <typename T>
inline static T &
view_back(View<T> &self)
{
	validate(self.count > 0, "[VIEW]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static const T &
view_back(const View<T> &self)
{
	validate(self.count > 0, "[VIEW]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static T *
begin(View<T> &self)
{
	return self.data;
}

template <typename T>
inline static const T *
begin(const View<T> &self)
{
	return self.data;
}

template <typename T>
inline static T *
end(View<T> &self)
{
	return self.data + self.count;
}

template <typename T>
inline static const T *
end(const View<T> &self)
{
	return self.data + self.count;
}

inline static U64
hash(const String_View &self)
{
	return hash_fnv_x32(self.data, self.count);
}