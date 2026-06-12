#pragma once

#include "core/export.h"
#include "core/memory/memory.h"

namespace memory
{
	struct Virtual_Allocator : Allocator
	{
		Memory_Block
		allocate(U64 size, U64 alignment) override;

		void
		deallocate(Memory_Block block) override;
	};

	CORE_API Virtual_Allocator *
	virtual_allocator_init();

	CORE_API void
	virtual_allocator_deinit(Virtual_Allocator *self);

	CORE_API Memory_Block
	virtual_allocator_allocate(Virtual_Allocator *self, U64 size, U64 alignment);

	CORE_API void
	virtual_allocator_deallocate(Virtual_Allocator *self, Memory_Block block);
}