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

struct Memory_Block
{
	void *data;
	U64 size;
};

namespace memory
{
	struct Allocator
	{
		virtual
		~Allocator() = default;

		virtual Memory_Block
		allocate(U64 size, U64 alignment) = 0;

		virtual void
		deallocate(Memory_Block block) = 0;

		virtual void
		clear() {}
	};

	CORE_API Allocator *
	heap_allocator();

	CORE_API Allocator *
	virtual_allocator();

	CORE_API Allocator *
	temp_allocator();

	inline static Memory_Block
	allocate(U64 size, U64 alignment)
	{
		auto allocator = heap_allocator();
		return allocator->allocate(size, alignment);
	}

	inline static Memory_Block
	allocate(Allocator *allocator, U64 size, U64 alignment)
	{
		return allocator->allocate(size, alignment);
	}

	template <typename T>
	inline static T *
	allocate()
	{
		return (T *)allocate(sizeof(T), alignof(T)).data;
	}

	template <typename T>
	inline static T *
	allocate(Allocator *allocator)
	{
		return (T *)allocate(allocator, sizeof(T), alignof(T)).data;
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

	inline static Memory_Block
	allocate_zeroed(U64 size, U64 alignment)
	{
		Memory_Block block = allocate(size, alignment);
		if (block.data != nullptr)
			::memset(block.data, 0, block.size);
		return block;
	}

	inline static Memory_Block
	allocate_zeroed(Allocator *allocator, U64 size, U64 alignment)
	{
		Memory_Block block = allocate(allocator, size, alignment);
		if (block.data != nullptr)
			::memset(block.data, 0, block.size);
		return block;
	}

	template <typename T>
	inline static T *
	allocate_zeroed()
	{
		return (T *)allocate_zeroed(sizeof(T), alignof(T)).data;
	}

	template <typename T>
	inline static T *
	allocate_zeroed(Allocator *allocator)
	{
		return (T *)allocate_zeroed(allocator, sizeof(T), alignof(T)).data;
	}

	inline static void
	deallocate(Memory_Block block)
	{
		auto allocator = heap_allocator();
		allocator->deallocate(block);
	}

	inline static void
	deallocate(Allocator *allocator, Memory_Block block)
	{
		allocator->deallocate(block);
	}

	template <typename T>
	inline static void
	deallocate(T *data)
	{
		deallocate(Memory_Block{data, sizeof(T)});
	}

	template <typename T>
	inline static void
	deallocate(Allocator *allocator, T *data)
	{
		deallocate(allocator, Memory_Block{data, sizeof(T)});
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