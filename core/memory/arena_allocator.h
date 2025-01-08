#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/memory/memory.h"

namespace memory
{
	inline static constexpr const u64 ARENA_ALLOCATOR_INITIAL_CAPACITY = 4 * 1024 * 1024ULL;

	struct Arena_Allocator : Allocator
	{
		struct Arena_Allocator_Context *ctx;

		Arena_Allocator(u64 initial_capacity = ARENA_ALLOCATOR_INITIAL_CAPACITY, Allocator *allocator = heap_allocator());

		~Arena_Allocator() override;

		void *
		allocate(u64 size) override;

		void
		deallocate(void *data) override;

		void
		clear() override;
	};

	CORE_API Arena_Allocator *
	arena_allocator_init(u64 initial_capacity = ARENA_ALLOCATOR_INITIAL_CAPACITY, Allocator *allocator = heap_allocator());

	CORE_API void
	arena_allocator_deinit(Arena_Allocator *self);

	CORE_API void *
	arena_allocator_allocate(Arena_Allocator *self, u64 size);

	CORE_API void
	arena_allocator_deallocate(Arena_Allocator *self, void *data);

	CORE_API void
	arena_allocator_clear(Arena_Allocator *self);

	CORE_API u64
	arena_allocator_get_used_size(Arena_Allocator *self);

	CORE_API u64
	arena_allocator_get_peak_size(Arena_Allocator *self);
}