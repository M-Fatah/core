#pragma once

#include "core/export.h"
#include "core/memory/memory.h"

namespace memory
{
	struct Heap_Allocator : Allocator
	{
#if DEBUG
		struct Heap_Allocator_Context *ctx;
#endif
		Heap_Allocator();

		~Heap_Allocator() override;

		void *
		allocate(u64 size) override;

		void
		deallocate(void *data) override;
	};

	CORE_API Heap_Allocator *
	heap_allocator_init();

	CORE_API void
	heap_allocator_deinit(Heap_Allocator *self);

	CORE_API void *
	heap_allocator_allocate(Heap_Allocator *self, u64 size);

	CORE_API void
	heap_allocator_deallocate(Heap_Allocator *self, void *data);
}