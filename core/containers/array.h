#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/reflect.h"
#include "core/memory/memory.h"

#include <type_traits>
#include <initializer_list>

template <typename T>
struct Array
{
	memory::Allocator *allocator;
	T *data;
	u64 count;
	u64 capacity;

	T &
	operator[](u64 index)
	{
		validate(index < count, "[ARRAY]: Access out of range.");
		return data[index];
	}

	const T &
	operator[](u64 index) const
	{
		validate(index < count, "[ARRAY]: Access out of range.");
		return data[index];
	}
};

template <typename T>
inline static Array<T>
array_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Array<T> {
		.allocator = allocator ? allocator : memory::heap_allocator(),
	};
}

template <typename T>
inline static Array<T>
array_init_with_capacity(u64 capacity, memory::Allocator *allocator = memory::heap_allocator())
{
	allocator = allocator ? allocator : memory::heap_allocator();
	return Array<T> {
		.allocator = allocator,
		.data = (T *)memory::allocate(allocator, capacity * sizeof(T)),
		.capacity = capacity
	};
}

template <typename T>
inline static Array<T>
array_init_with_count(u64 count, memory::Allocator *allocator = memory::heap_allocator())
{
	allocator = allocator ? allocator : memory::heap_allocator();
	return Array<T> {
		.allocator = allocator,
		.data = (T *)memory::allocate(allocator, count * sizeof(T)),
		.count = count,
		.capacity = count
	};
}

template <typename T>
inline static Array<T>
array_init_from(const T *first, const T *last, memory::Allocator *allocator = memory::heap_allocator())
{
	Array<T> self = array_init_with_capacity<T>(last - first, allocator);
	for (const T *it = first; it != last; ++it)
		self[self.count++] = *it;
	return self;
}

template <typename T>
inline static Array<T>
array_init_from(std::initializer_list<T> values, memory::Allocator *allocator = memory::heap_allocator())
{
	return array_init_from(values.begin(), values.end(), allocator);
}

template <typename T>
inline static Array<T>
array_copy(const Array<T> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	Array<T> copy = array_init_with_count<T>(self.count, allocator);
	for (u64 i = 0; i < self.count; ++i)
		copy[i] = self[i];
	return copy;
}

template <typename T>
inline static void
array_deinit(Array<T> &self)
{
	if (self.capacity && self.allocator)
		memory::deallocate(self.allocator, self.data);
	self = Array<T>{.allocator = self.allocator};
}

template <typename T>
inline static void
array_reserve(Array<T> &self, u64 added_capacity)
{
	if (self.count + added_capacity < self.capacity)
		return;

	if (self.allocator == nullptr)
		self.allocator = memory::heap_allocator();

	self.capacity += added_capacity;

	T *data = (T *)memory::allocate(self.allocator, self.capacity * sizeof(T));
	for (u64 i = 0; i < self.count; ++i)
		data[i] = self[i];
	memory::deallocate(self.allocator, self.data);

	self.data = data;
}

template <typename T>
inline static void
array_resize(Array<T> &self, u64 new_count)
{
	if (new_count > self.count)
		array_reserve(self, new_count - self.count);
	self.count = new_count;
}

template <typename T, typename R>
inline static void
array_push(Array<T> &self, const R &value)
{
	if ((self.count + 1) >= self.capacity)
		array_reserve(self, self.capacity > 1 ? self.capacity / 2 : 8);
	self[self.count++] = (T)value;
}

template <typename T>
inline static void
array_push(Array<T> &self, const T &value, u64 count)
{
	u64 i = self.count;
	array_resize(self, self.count + count);
	for (; i < self.count; ++i)
		self[i] = value;
}

template <typename T>
inline static T
array_pop(Array<T> &self)
{
	validate(self.count > 0, "[ARRAY]: Trying to pop from an empty array.");
	T last = self[self.count - 1];
	--self.count;
	return last;
}

template <typename T>
inline static void
array_remove(Array<T> &self, u64 index)
{
	validate(index < self.count, "[ARRAY]: Access out of range.");
	if ((index + 1) != self.count)
	{
		T temp = self[self.count - 1];
		self[self.count - 1] = self[index];
		self[index] = temp;
	}
	--self.count;
}

template <typename T, typename P>
inline static void
array_remove_if(Array<T> &self, P &&predicate)
{
	for (u64 i = 0; i < self.count; ++i)
	{
		if (predicate(self[i]))
		{
			array_remove(self, i);
			--i;
		}
	}
}

template <typename T>
inline static void
array_remove_ordered(Array<T> &self, u64 index)
{
	validate(index < self.count, "[ARRAY]: Access out of range.");
	::memmove(self.data + index, self.data + index + 1, (self.count - index - 1) * sizeof(T));
	--self.count;
}

template <typename T, typename P>
inline static void
array_remove_ordered_if(Array<T> &self, P &&predicate)
{
	for (u64 i = 0; i < self.count; ++i)
	{
		if (predicate(self[i]))
		{
			array_remove_ordered(self, i);
			--i;
		}
	}
}

template <typename T>
inline static void
array_append(Array<T> &self, const Array<T> &other)
{
	u64 old_count = self.count;
	array_resize(self, self.count + other.count);
	for (u64 i = 0; i < other.count; ++i)
		self[old_count + i] = other[i];
}

template <typename T, typename R>
inline static void
array_fill(Array<T> &self, const R &value)
{
	for (u64 i = 0; i < self.count; ++i)
		self[i] = (T)value;
}

template <typename T>
inline static void
array_clear(Array<T> &self)
{
	self.count = 0;
}

template <typename T>
inline static bool
array_is_empty(const Array<T> &self)
{
	return self.count == 0;
}

template <typename T>
inline static T &
array_first(Array<T> &self)
{
	validate(self.count > 0, "[ARRAY]: Count is 0.");
	return self[0];
}

template <typename T>
inline static T &
array_last(Array<T> &self)
{
	validate(self.count > 0, "[ARRAY]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static T *
begin(Array<T> &self)
{
	return self.data;
}

template <typename T>
inline static const T *
begin(const Array<T> &self)
{
	return self.data;
}

template <typename T>
inline static T *
end(Array<T> &self)
{
	return self.data + self.count;
}

template <typename T>
inline static const T *
end(const Array<T> &self)
{
	return self.data + self.count;
}

template <typename T>
inline static Array<T>
clone(const Array<T> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	Array<T> copy = array_copy(self, allocator);
	if constexpr (std::is_class_v<T>)
		for (T &element : copy)
			element = clone(element);
	return copy;
}

template <typename T>
inline static void
destroy(Array<T> &self)
{
	if constexpr (std::is_class_v<T>)
		for (T &element : self)
			destroy(element);
	array_deinit(self);
}

template <typename T>
TYPE_OF(Array<T>, data, count, capacity, allocator)