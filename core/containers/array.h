#pragma once

#include "core/defines.h"
#include "core/assert.h"
#include "core/formatter.h"
#include "core/memory/memory.h"
#include "core/serialization/serializer.h"

#include <type_traits>
#include <initializer_list>

template <typename T>
struct Array
{
	T *data;
	u64 count;
	u64 capacity;
	memory::Allocator *allocator;

	T &
	operator[](u64 index)
	{
		ASSERT(index < count, "[ARRAY]: Access out of range.");
		return data[index];
	}

	const T &
	operator[](u64 index) const
	{
		ASSERT(index < count, "[ARRAY]: Access out of range.");
		return data[index];
	}
};

template <typename T>
inline static Array<T>
array_init(memory::Allocator *allocator = memory::heap_allocator())
{
	Array<T> self = {};
	self.allocator = allocator;
	return self;
}

template <typename T>
inline static Array<T>
array_with_capacity(u64 capacity, memory::Allocator *allocator = memory::heap_allocator())
{
	Array<T> self = {};
	self.allocator = allocator;
	self.data = (T*)memory::allocate(self.allocator, capacity * sizeof(T));
	self.capacity = capacity;
	return self;
}

template <typename T>
inline static Array<T>
array_with_count(u64 count, memory::Allocator *allocator = memory::heap_allocator())
{
	auto self = array_with_capacity<T>(count, allocator);
	self.count = count;
	return self;
}

template <typename T>
inline static Array<T>
array_from(const T *first, const T *last, memory::Allocator *allocator = memory::heap_allocator())
{
	auto self = array_with_capacity<T>(last - first, allocator);
	for (const T *it = first; it != last; ++it)
		self[self.count++] = *it;
	return self;
}

template <typename T>
inline static Array<T>
array_from(std::initializer_list<T> values, memory::Allocator *allocator = memory::heap_allocator())
{
	return array_from(values.begin(), values.end(), allocator);
}

template <typename T>
inline static Array<T>
array_copy(const Array<T> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	auto copy = array_with_count<T>(self.count, allocator);
	for (u64 i = 0; i < self.count; ++i)
		copy[i] = self[i];
	return copy;
}

template <typename T>
inline static void
array_deinit(Array<T> &self)
{
	if(self.allocator)
		memory::deallocate(self.allocator, self.data);
	self = {};
}

template <typename T>
inline static void
array_reserve(Array<T> &self, u64 added_capacity)
{
	if(self.count + added_capacity < self.capacity)
		return;

	if (self.allocator == nullptr)
		self.allocator = memory::heap_allocator();

	self.capacity += added_capacity;
	auto old_data = self.data;
	self.data = (T *)memory::allocate(self.allocator, self.capacity * sizeof(T));
	for (u64 i = 0; i < self.count; ++i)
		self[i] = old_data[i];
	memory::deallocate(self.allocator, old_data);
}

template <typename T>
inline static void
array_resize(Array<T> &self, u64 new_count)
{
	if(new_count > self.count)
		array_reserve(self, new_count - self.count);
	self.count = new_count;
}

template <typename T, typename R>
inline static void
array_push(Array<T> &self, const R &value)
{
	if((self.count + 1) >= self.capacity)
		array_reserve(self, self.capacity > 1 ? self.capacity / 2 : 8);
	self.data[self.count++] = (T)value;
}

template <typename T>
inline static void
array_push(Array<T> &self, const T &value, u64 count)
{
	u64 i = self.count;
	array_resize(self, self.count + count);
	for(; i < self.count; ++i)
		self.data[i] = value;
}

template <typename T>
inline static T
array_pop(Array<T> &self)
{
	ASSERT(self.count > 0, "[ARRAY]: Trying to pop from a 0 count array.");
	return self.data[--self.count];
}

template <typename T>
inline static void
array_remove(Array<T> &self, u64 index)
{
	ASSERT(index < self.count, "[ARRAY]: Access out of range.");
	if((index + 1) != self.count)
	{
		T temp = self.data[self.count - 1];
		self.data[self.count - 1] = self.data[index];
		self.data[index] = temp;
	}
	--self.count;
}

template <typename T, typename P>
inline static void
array_remove_if(Array<T> &self, P &&predicate)
{
	for (u64 i = 0; i < self.count; ++i)
	{
		if (predicate(self[i]) == true)
		{
			array_remove(self, i);
			--i;
		}
	}
}

template <typename T>
inline static void
array_append(Array<T> &self, const Array<T> &other)
{
	auto old_count = self.count;
	array_resize(self, self.count + other.count);
	for (u64 i = 0; i < other.count; ++i)
		self[old_count + i] = other[i];
}

template <typename T, typename R>
inline static void
array_fill(Array<T> &self, const R &value)
{
	for(u64 i = 0; i < self.count; ++i)
		self.data[i] = value;
}

template <typename T>
inline static void
array_clear(Array<T> &self)
{
	self.count = 0;
}

template <typename T>
inline static bool
array_is_empty(Array<T> &self)
{
	return self.count == 0;
}

template <typename T>
inline static T &
array_first(Array<T> &self)
{
	ASSERT(self.count > 0, "[ARRAY]: Count is 0.");
	return self[0];
}

template <typename T>
inline static T &
array_last(Array<T> &self)
{
	ASSERT(self.count > 0, "[ARRAY]: Count is 0.");
	return self[self.count - 1];
}

template <typename T>
inline static const T *
begin(const Array<T> &self)
{
	return self.data;
}

template <typename T>
inline static T *
begin(Array<T> &self)
{
	return self.data;
}

template <typename T>
inline static const T *
end(const Array<T> &self)
{
	return self.data + self.count;
}

template <typename T>
inline static T *
end(Array<T> &self)
{
	return self.data + self.count;
}

template <typename T>
inline static Array<T>
clone(const Array<T> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	if constexpr (std::is_compound_v<T>)
	{
		auto copy = array_with_count<T>(self.count, allocator);
		for (u64 i = 0; i < self.count; ++i)
			copy[i] = clone(self[i], allocator);
		return copy;
	}
	else
	{
		return array_copy(self, allocator);
	}
}

template <typename T>
inline static void
destroy(Array<T> &self)
{
	if constexpr (std::is_compound_v<T>)
	{
		for (u64 i = 0; i < self.count; ++i)
			destroy(self[i]);
	}
	array_deinit(self);
}

// TODO:
template <typename T>
inline static void
serialize(Serializer *serializer, const char *name, const Array<T> &self)
{
	serializer->begin(SERIALIZER_BEGIN_STATE_ARRAY, name);
	serialize(serializer, "count", self.count);
	for (u64 i = 0; i < self.count; ++i)
		serialize(serializer, "", self[i]);
	serializer->end();
}

// TODO:
template <typename T>
inline static void
deserialize(Serializer *serializer, const char *, Array<T> &self)
{
	u64 count = 0;
	deserialize(serializer, "count", count);
	array_resize(self, count);
	for (u64 i = 0; i < count; ++i)
		deserialize(serializer, "", self[i]);
}

template <typename T>
inline static void
format(Formatter *formatter, const Array<T> &self)
{
	format(formatter, "[{}] {{ ", self.count);
	for (u64 i = 0; i < self.count; ++i)
	{
		if (i != 0)
			format(formatter, ", ");
		format(formatter, "{}", self[i]);
	}
	format(formatter, " }}");
}