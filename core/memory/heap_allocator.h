#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/memory/memory.h"

namespace memory
{
	struct Heap_Allocator : Allocator
	{
		#if DEBUG
			struct Heap_Allocator_Context *ctx;
		#endif

		Heap_Allocator();

		~Heap_Allocator();

		Memory_Block
		allocate(U64 size, U64 alignment) override;

		void
		deallocate(Memory_Block block) override;
	};

	CORE_API Heap_Allocator *
	heap_allocator_init();

	CORE_API void
	heap_allocator_deinit(Heap_Allocator *self);

	CORE_API Memory_Block
	heap_allocator_allocate(Heap_Allocator *self, U64 size, U64 alignment);

	CORE_API void
	heap_allocator_deallocate(Heap_Allocator *self, Memory_Block block);
}