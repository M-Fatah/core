#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/memory/memory.h"

namespace memory
{
	static constexpr const U64 ARENA_ALLOCATOR_INITIAL_CAPACITY = 4 * 1024 * 1024ULL;

	struct Arena_Allocator : Allocator
	{
		struct Arena_Allocator_Context *ctx;

		Arena_Allocator(U64 initial_capacity = ARENA_ALLOCATOR_INITIAL_CAPACITY, Allocator *allocator = heap_allocator());

		~Arena_Allocator() override;

		Memory_Block
		allocate(U64 size, U64 alignment) override;

		void
		deallocate(Memory_Block block) override;

		void
		clear() override;
	};

	CORE_API Arena_Allocator *
	arena_allocator_init(U64 initial_capacity = ARENA_ALLOCATOR_INITIAL_CAPACITY, Allocator *allocator = heap_allocator());

	CORE_API void
	arena_allocator_deinit(Arena_Allocator *self);

	CORE_API Memory_Block
	arena_allocator_allocate(Arena_Allocator *self, U64 size, U64 alignment);

	CORE_API void
	arena_allocator_deallocate(Arena_Allocator *self, Memory_Block block);

	CORE_API void
	arena_allocator_clear(Arena_Allocator *self);

	CORE_API U64
	arena_allocator_get_used_size(Arena_Allocator *self);

	CORE_API U64
	arena_allocator_get_peak_size(Arena_Allocator *self);
}