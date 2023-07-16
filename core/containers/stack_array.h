#pragma once

#include "core/assert.h"
#include "core/defines.h"

#include <initializer_list>

/*
	TODO:
	- [ ] Support designated initialization.
*/

template <typename T, u64 N>
struct Stack_Array
{
	T data[N];
	u64 count;

	Stack_Array() : data(), count(0) {}

	Stack_Array(const T (&values)[N])
	{
		::memcpy(data, values, sizeof(T) * N);
		count = N;
	}

	T &
	operator[](u64 index)
	{
		ASSERT(index < count, "[STACK_ARRAY]: Access out of range.");
		return data[index];
	}

	const T &
	operator[](u64 index) const
	{
		ASSERT(index < count, "[STACK_ARRAY]: Access out of range.");
		return data[index];
	}
};

template <typename T, u64 N>
inline static void
stack_array_push(Stack_Array<T, N> &self, const T &value)
{
	ASSERT(self.count < N, "[STACK_ARRAY]: Access out of range.");
	self.data[self.count++] = value;
}

template <typename T, u64 N>
inline static T
stack_array_pop(Stack_Array<T, N> &self)
{
	ASSERT(self.count > 0, "[STACK_ARRAY]: Trying to pop from a 0 count array.");
	return self.data[--self.count];
}

template <typename T, u64 N>
inline static void
stack_array_clear(Stack_Array<T, N> &self)
{
	self.count = 0;
}

template <typename T, u64 N>
inline static const T *
begin(const Stack_Array<T, N> &self)
{
	return self.data;
}

template <typename T, u64 N>
inline static T *
begin(Stack_Array<T, N> &self)
{
	return self.data;
}

template <typename T, u64 N>
inline static const T *
end(const Stack_Array<T, N> &self)
{
	return self.data + self.count;
}

template <typename T, u64 N>
inline static T *
end(Stack_Array<T, N> &self)
{
	return self.data + self.count;
}