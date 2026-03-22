#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/memory/memory.h"

#include <string.h>
#include <type_traits>

template <typename T>
struct Ring_Buffer
{
	memory::Allocator *allocator;
	T *data;
	u64 count;
	u64 capacity;
	u64 head;

	inline T &
	operator[](u64 index)
	{
		validate(index < count, "[RING_BUFFER]: Access out of range.");
		return data[(head + index) % capacity];
	}

	inline const T &
	operator[](u64 index) const
	{
		validate(index < count, "[RING_BUFFER]: Access out of range.");
		return data[(head + index) % capacity];
	}
};

template <typename T>
inline static Ring_Buffer<T>
ring_buffer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Ring_Buffer<T> {
		.allocator = allocator,
		.data = nullptr,
		.count = 0,
		.capacity = 0,
		.head = 0,
	};
}

template <typename T>
inline static Ring_Buffer<T>
ring_buffer_copy(const Ring_Buffer<T> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	Ring_Buffer<T> copy = ring_buffer_init<T>(allocator);
	ring_buffer_reserve(copy, self.count);
	copy.count = self.count;
	for (u64 i = 0; i < self.count; ++i)
		copy[i] = self[i];
	return copy;
}

template <typename T>
inline static void
ring_buffer_deinit(Ring_Buffer<T> &self)
{
	if (self.data == nullptr)
		return;

	memory::deallocate(self.allocator, self.data);
	self = Ring_Buffer<T>{.allocator = self.allocator};
}

template <typename T>
inline static void
ring_buffer_reserve(Ring_Buffer<T> &self, u64 added_capacity)
{
	if (self.count + added_capacity <= self.capacity)
		return;

	if (self.allocator == nullptr)
		self.allocator = memory::heap_allocator();

	u64 next_cap     = (u64)(self.capacity * 1.5f);
	u64 needed_cap   = self.count + added_capacity;
	u64 new_capacity = next_cap > needed_cap ? next_cap : needed_cap;

	T *new_data = (T *)memory::allocate(self.allocator, new_capacity * sizeof(T));

	if (self.count)
	{
		const u64 first_chunk = self.capacity - self.head;
		if (first_chunk >= self.count)
		{
			::memcpy(new_data, self.data + self.head, self.count * sizeof(T));
		}
		else
		{
			::memcpy(new_data, self.data + self.head, first_chunk * sizeof(T));
			::memcpy(new_data + first_chunk, self.data, (self.count - first_chunk) * sizeof(T));
		}
	}

	if (self.capacity)
		memory::deallocate(self.allocator, self.data);

	self.data     = new_data;
	self.capacity = new_capacity;
	self.head     = 0;
}

template <typename T, typename R>
inline static void
ring_buffer_push_front(Ring_Buffer<T> &self, const R &data)
{
	if (self.count == self.capacity)
		ring_buffer_reserve(self, self.capacity ? 1 : 8);
	self.head = self.head ? self.head - 1 : self.capacity - 1;
	self.data[self.head] = data;
	++self.count;
}

template <typename T, typename R>
inline static void
ring_buffer_push_back(Ring_Buffer<T> &self, const R &data)
{
	if (self.count == self.capacity)
		ring_buffer_reserve(self, self.capacity ? 1 : 8);
	self.data[(self.head + self.count) % self.capacity] = data;
	++self.count;
}

template <typename T>
inline static void
ring_buffer_pop_front(Ring_Buffer<T> &self)
{
	validate(self.count > 0, "[RING_BUFFER]: Count is 0.");
	self.head = (self.head + 1) % self.capacity;
	--self.count;
}

template <typename T>
inline static void
ring_buffer_pop_back(Ring_Buffer<T> &self)
{
	validate(self.count > 0, "[RING_BUFFER]: Count is 0.");
	--self.count;
}

template <typename T>
inline static T &
ring_buffer_front(Ring_Buffer<T> &self)
{
	validate(self.count > 0, "[RING_BUFFER]: Count is 0.");
	return self.data[self.head];
}

template <typename T>
inline static const T &
ring_buffer_front(const Ring_Buffer<T> &self)
{
	validate(self.count > 0, "[RING_BUFFER]: Count is 0.");
	return self.data[self.head];
}

template <typename T>
inline static T &
ring_buffer_back(Ring_Buffer<T> &self)
{
	validate(self.count > 0, "[RING_BUFFER]: Count is 0.");
	return self.data[(self.head + self.count - 1) % self.capacity];
}

template <typename T>
inline static const T &
ring_buffer_back(const Ring_Buffer<T> &self)
{
	validate(self.count > 0, "[RING_BUFFER]: Count is 0.");
	return self.data[(self.head + self.count - 1) % self.capacity];
}

template <typename T>
inline static void
ring_buffer_clear(Ring_Buffer<T> &self)
{
	self.head  = 0;
	self.count = 0;
}

template <typename T>
inline static bool
ring_buffer_is_empty(const Ring_Buffer<T> &self)
{
	return self.count == 0;
}

template <typename T>
inline static Ring_Buffer<T>
clone(const Ring_Buffer<T> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	Ring_Buffer<T> copy = ring_buffer_copy(self, allocator);
	if constexpr (std::is_class_v<T>)
		for (u64 i = 0; i < self.count; ++i)
			copy[i] = clone(copy[i]);
	return copy;
}

template <typename T>
inline static void
destroy(Ring_Buffer<T> &self)
{
	if constexpr (std::is_class_v<T>)
		for (u64 i = 0; i < self.count; ++i)
			destroy(self[i]);
	ring_buffer_deinit(self);
}