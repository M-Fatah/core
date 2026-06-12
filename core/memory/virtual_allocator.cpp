#include "core/memory/virtual_allocator.h"

#include "core/log.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/platform/platform.h"

namespace memory
{
	Memory_Block
	Virtual_Allocator::allocate(U64 size, U64 alignment)
	{
		if (size == 0)
			return Memory_Block{};

		validate(u64_is_power_of_two(alignment)                      , "[ALLOCATOR][VIRTUAL]: Alignment must be a non-zero power of two.");
		validate(alignment <= platform_virtual_memory_get_page_size(), "[ALLOCATOR][VIRTUAL]: Alignment cannot exceed platform page size.");

		U64 reserved_size = platform_virtual_memory_page_align(size);
		Memory_Block reserved = platform_virtual_memory_reserve(reserved_size);
		if (reserved.data == nullptr)
		{
			log_fatal("[ALLOCATOR][VIRTUAL]: Could not reserve virtual memory with size {}.", reserved_size);
		}

		if (!platform_virtual_memory_commit(reserved))
		{
			platform_virtual_memory_release(reserved);
			log_fatal("[ALLOCATOR][VIRTUAL]: Could not commit virtual memory with size {}.", reserved_size);
		}

		return Memory_Block{.data = reserved.data, .size = size};
	}

	void
	Virtual_Allocator::deallocate(Memory_Block block)
	{
		if (block.data == nullptr)
			return;

		platform_virtual_memory_release(Memory_Block {
			.data = block.data,
			.size = platform_virtual_memory_page_align(block.size)
		});
	}

	Virtual_Allocator *
	virtual_allocator_init()
	{
		return allocate_and_call_constructor<Virtual_Allocator>();
	}

	void
	virtual_allocator_deinit(Virtual_Allocator *self)
	{
		deallocate_and_call_destructor(self);
	}

	Memory_Block
	virtual_allocator_allocate(Virtual_Allocator *self, U64 size, U64 alignment)
	{
		return self->allocate(size, alignment);
	}

	void
	virtual_allocator_deallocate(Virtual_Allocator *self, Memory_Block block)
	{
		self->deallocate(block);
	}
}