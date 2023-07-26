#include "core/memory/memory.h"

#include "core/memory/heap_allocator.h"
#include "core/memory/arena_allocator.h"

namespace memory
{
	struct Context
	{
		Heap_Allocator *heap_allocator;
		Arena_Allocator *temp_allocator;

		Context()
		{
			heap_allocator = heap_allocator_init();
			temp_allocator = arena_allocator_init(ARENA_ALLOCATOR_INITIAL_CAPACITY, heap_allocator);
		}

		~Context()
		{
			arena_allocator_deinit(temp_allocator);
			heap_allocator_deinit(heap_allocator);
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
		return ctx->heap_allocator;
	}

	Allocator *
	temp_allocator()
	{
		Context *ctx = context_get();
		return ctx->temp_allocator;
	}
}