#pragma once

#include "core/validate.h"
#include "core/defines.h"

#include <initializer_list>

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

	template <u64 NN>
	Stack_Array(const T (&values)[NN])
	{
		::memcpy(data, values, sizeof(T) * NN);
		count = NN;
	}

	T &
	operator[](u64 index)
	{
		validate(index < count, "[STACK_ARRAY]: Access out of range.");
		return data[index];
	}

	const T &
	operator[](u64 index) const
	{
		validate(index < count, "[STACK_ARRAY]: Access out of range.");
		return data[index];
	}
};

template <typename T, u64 N>
inline static void
stack_array_push(Stack_Array<T, N> &self, const T &value)
{
	validate(self.count < N, "[STACK_ARRAY]: Access out of range.");
	self.data[self.count++] = value;
}

template <typename T, u64 N>
inline static T
stack_array_pop(Stack_Array<T, N> &self)
{
	validate(self.count > 0, "[STACK_ARRAY]: Trying to pop from a 0 count array.");
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