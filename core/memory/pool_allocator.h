#pragma once

#include "core/export.h"
#include "core/memory/memory.h"

namespace memory
{
	struct Pool_Allocator : Allocator
	{
		struct Pool_Allocator_Context *ctx;

		Pool_Allocator(U64 chunk_size, U64 chunk_count);

		~Pool_Allocator() override;

		void *
		allocate(U64 size = 0, U64 alignment = alignof(void *)) override;

		void
		deallocate(void *data) override;
	};

	CORE_API Pool_Allocator *
	pool_allocator_init(U64 chunk_size, U64 chunk_count);

	CORE_API void
	pool_allocator_deinit(Pool_Allocator *self);

	CORE_API void *
	pool_allocator_allocate(Pool_Allocator *self);

	CORE_API void
	pool_allocator_deallocate(Pool_Allocator *self, void *data);
}