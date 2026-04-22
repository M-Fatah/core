#pragma once

#include "core/export.h"
#include "core/defines.h"

#include <new>
#include <utility>
#include <string.h>

/*
	TODO:
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
		allocate(U64 size, U64 alignment) = 0;

		virtual void
		deallocate(void *data) = 0;

		virtual void
		clear() {}
	};

	CORE_API Allocator *
	heap_allocator();

	CORE_API Allocator *
	temp_allocator();

	// Untyped allocation — alignment required (no default).
	inline static void *
	allocate(U64 size, U64 alignment)
	{
		auto allocator = heap_allocator();
		return allocator->allocate(size, alignment);
	}

	inline static void *
	allocate(Allocator *allocator, U64 size, U64 alignment)
	{
		return allocator->allocate(size, alignment);
	}

	// Typed allocation — alignment auto from alignof(T), count optional.
	template <typename T>
	inline static T *
	allocate(U64 count = 1)
	{
		return (T *)allocate(sizeof(T) * count, alignof(T));
	}

	template <typename T>
	inline static T *
	allocate(Allocator *allocator, U64 count = 1)
	{
		return (T *)allocate(allocator, sizeof(T) * count, alignof(T));
	}

	// Constructor-calling typed allocation.
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

	// Zeroed allocation — mirrors the non-zeroed family.
	inline static void *
	allocate_zeroed(U64 size, U64 alignment)
	{
		void *data = allocate(size, alignment);
		if (data != nullptr)
			::memset(data, 0, size);
		return data;
	}

	inline static void *
	allocate_zeroed(Allocator *allocator, U64 size, U64 alignment)
	{
		void *data = allocate(allocator, size, alignment);
		if (data != nullptr)
			::memset(data, 0, size);
		return data;
	}

	template <typename T>
	inline static T *
	allocate_zeroed(U64 count = 1)
	{
		return (T *)allocate_zeroed(sizeof(T) * count, alignof(T));
	}

	template <typename T>
	inline static T *
	allocate_zeroed(Allocator *allocator, U64 count = 1)
	{
		return (T *)allocate_zeroed(allocator, sizeof(T) * count, alignof(T));
	}

	// Deallocation — unchanged signature (allocator tracks alignment internally).
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
