#include "core/memory/arena_allocator.h"

#include "core/log.h"
#include "core/platform/platform.h"

// TODO: Remove and use logger.
#include <stdio.h>
#include <inttypes.h>

namespace memory
{
	struct Arena_Allocator_Context
	{
		memory::Allocator *allocator;
		Platform_Memory memory;
		u64 used_size;
		u64 peak_size;
	};

	Arena_Allocator::Arena_Allocator(u64 initial_capacity, memory::Allocator *allocator)
	{
		Arena_Allocator *self = this;
		self->ctx = memory::allocate_zeroed<Arena_Allocator_Context>(allocator);
		self->ctx->allocator = allocator;
		self->ctx->memory    = platform_virtual_memory_reserve(nullptr, initial_capacity);
		self->ctx->used_size = 0;
		self->ctx->peak_size = 0;

		validate(self->ctx->memory.size, "[ALLOCATOR][ARENA]: Could not init arena allocator.");
	}

	Arena_Allocator::~Arena_Allocator()
	{
		Arena_Allocator *self = this;
		platform_virtual_memory_decommit(self->ctx->memory);
		platform_virtual_memory_release(self->ctx->memory);
		memory::deallocate(self->ctx->allocator, self->ctx);
	}

	void *
	Arena_Allocator::allocate(u64 size)
	{
		Arena_Allocator *self = this;

		validate(self->ctx->used_size + size < self->ctx->memory.size, "[ALLOCATOR][ARENA]: Could not allocate memory; arena allocator is full.");

		Platform_Memory memory = {
			.ptr = self->ctx->memory.ptr + self->ctx->used_size,
			.size = size
		};
		platform_virtual_memory_commit(memory);

		self->ctx->used_size += size;
		self->ctx->peak_size = self->ctx->used_size > self->ctx->peak_size ? self->ctx->used_size : self->ctx->peak_size;

		return memory.ptr;
	}

	void
	Arena_Allocator::deallocate(void *)
	{

	}

	void
	Arena_Allocator::clear()
	{
		Arena_Allocator *self = this;
		platform_virtual_memory_decommit(self->ctx->memory);
		self->ctx->used_size = 0;
	}

	Arena_Allocator *
	arena_allocator_init(u64 initial_capacity, memory::Allocator *allocator)
	{
		return memory::allocate_and_call_constructor<Arena_Allocator>(allocator, initial_capacity, allocator);
	}

	void
	arena_allocator_deinit(Arena_Allocator *self)
	{
		memory::deallocate_and_call_destructor(self->ctx->allocator, self);
	}

	void *
	arena_allocator_allocate(Arena_Allocator *self, u64 size)
	{
		return self->allocate(size);
	}

	void
	arena_allocator_deallocate(Arena_Allocator *self, void *data)
	{
		self->deallocate(data);
	}

	void
	arena_allocator_clear(Arena_Allocator *self)
	{
		self->clear();
	}

	u64
	arena_allocator_get_used_size(Arena_Allocator *self)
	{
		return self->ctx->used_size;
	}

	u64
	arena_allocator_get_peak_size(Arena_Allocator *self)
	{
		return self->ctx->peak_size;
	}
}