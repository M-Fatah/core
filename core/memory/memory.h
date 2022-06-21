#pragma once

#include "core/export.h"
#include "core/defines.h"

#include <new>
#include <utility>
#include <string.h>

/*
	TODO:
	- [ ] Memory alignment.
	- [ ] Rename this file to allocator.h?
	- [ ] Add meta allocator to arena and pool allocators?
*/

namespace memory
{
	struct Allocator
	{
		virtual
		~Allocator() = default;

		virtual void *
		allocate(u64 size) = 0;

		virtual void
		deallocate(void *data) = 0;

		virtual void
		clear() {}
	};

	CORE_API Allocator *
	heap_allocator();

	CORE_API Allocator *
	temp_allocator();

	inline static void *
	allocate(u64 size)
	{
		auto allocator = heap_allocator();
		return allocator->allocate(size);
	}

	inline static void *
	allocate(Allocator *allocator, u64 size)
	{
		return allocator->allocate(size);
	}

	template <typename T>
	inline static T *
	allocate()
	{
		return (T *)allocate(sizeof(T));
	}

	template <typename T>
	inline static T *
	allocate(Allocator *allocator)
	{
		return (T *)allocate(allocator, sizeof(T));
	}

	template <typename T, typename ...TArgs>
	inline static T *
	allocate_and_call_constructor(TArgs &&...args)
	{
		T *data = allocate<T>();
		::new (data) T(std::forward<TArgs>(args)...);
		return data;
	}

	template <typename T, typename ...TArgs>
	inline static T *
	allocate_and_call_constructor(Allocator *allocator, TArgs &&...args)
	{
		T *data = allocate<T>(allocator);
		::new (data) T(std::forward<TArgs>(args)...);
		return data;
	}

	inline static void *
	allocate_zeroed(u64 size)
	{
		void *data = allocate(size);
		::memset(data, 0, size);
		return data;
	}

	inline static void *
	allocate_zeroed(Allocator *allocator, u64 size)
	{
		void *data = allocate(allocator, size);
		::memset(data, 0, size);
		return data;
	}

	template <typename T>
	inline static T *
	allocate_zeroed()
	{
		return (T *)allocate_zeroed(sizeof(T));
	}

	template <typename T>
	inline static T *
	allocate_zeroed(Allocator *allocator)
	{
		return (T *)allocate_zeroed(allocator, sizeof(T));
	}

	inline static void
	deallocate(void *data)
	{
		auto allocator = heap_allocator();
		allocator->deallocate(data);
	}

	inline static void
	deallocate(Allocator *allocator, void *data)
	{
		allocator->deallocate(data);
	}

	template <typename T>
	inline static void
	deallocate_and_call_destructor(T *data)
	{
		data->~T();
		deallocate(data);
	}

	template <typename T>
	inline static void
	deallocate_and_call_destructor(Allocator *allocator, T *data)
	{
		data->~T();
		deallocate(allocator, data);
	}
}