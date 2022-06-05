#pragma once

#include "core/export.h"
#include "core/memory/memory.h"

namespace memory
{
	struct Pool_Allocator : Allocator
	{
		struct Pool_Allocator_Context *ctx;

		Pool_Allocator(u64 chunk_size, u64 chunk_count);

		~Pool_Allocator() override;

		void *
		allocate(u64 size = 0) override;

		void
		deallocate(void *data) override;
	};

	CORE_API Pool_Allocator *
	pool_allocator_init(u64 chunk_size, u64 chunk_count);

	CORE_API void
	pool_allocator_deinit(Pool_Allocator *self);

	CORE_API void *
	pool_allocator_allocate(Pool_Allocator *self);

	CORE_API void
	pool_allocator_deallocate(Pool_Allocator *self, void *data);
}