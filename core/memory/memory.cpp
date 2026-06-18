#include "core/memory/memory.h"

#include "core/memory/heap_allocator.h"
#include "core/memory/arena_allocator.h"

namespace memory
{
	struct Context
	{
		Arena_Allocator temp_allocator;
		Heap_Allocator  heap_allocator;
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
	temp_allocator()
	{
		Context *ctx = context_get();
		return &ctx->temp_allocator;
	}

	void
	temp_allocator_clear()
	{
		Context *ctx = context_get();
		ctx->temp_allocator.clear();
	}

	Arena_Allocator_Mark
	temp_allocator_mark()
	{
		Context *ctx = context_get();
		return arena_allocator_mark(&ctx->temp_allocator);
	}

	void
	temp_allocator_reset_to_mark(Arena_Allocator_Mark mark)
	{
		Context *ctx = context_get();
		arena_allocator_reset_to_mark(&ctx->temp_allocator, mark);
	}
}