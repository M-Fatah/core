#include "core/memory/memory.h"

#include "core/memory/heap_allocator.h"
#include "core/memory/arena_allocator.h"
#include "core/memory/virtual_allocator.h"

namespace memory
{
	struct Context
	{
		Heap_Allocator    heap_allocator;
		Virtual_Allocator virtual_allocator;
		Arena_Allocator   temp_allocator;

		Context()
			: temp_allocator(ARENA_ALLOCATOR_INITIAL_CAPACITY, &virtual_allocator)
		{
		}
	};

	static Context *
	context_get()
	{
		static Context self;
		return &self;
	}

	Allocator *
	heap_allocator()
	{
		Context *ctx = context_get();
		return &ctx->heap_allocator;
	}

	Allocator *
	virtual_allocator()
	{
		Context *ctx = context_get();
		return &ctx->virtual_allocator;
	}

	Allocator *
	temp_allocator()
	{
		Context *ctx = context_get();
		return &ctx->temp_allocator;
	}
}