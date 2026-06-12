#include "core/memory/pool_allocator.h"

#include "core/log.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/memory/arena_allocator.h"

namespace memory
{
	struct Pool_Allocator_Node
	{
		Pool_Allocator_Node *next;
	};

	struct Pool_Allocator_Context
	{
		Arena_Allocator *arena;
		Pool_Allocator_Node *head;
		U64 chunk_size;
	};

	Pool_Allocator::Pool_Allocator(U64 chunk_size, U64 chunk_count)
	{
		Pool_Allocator *self = this;
		self->ctx = memory::allocate_zeroed<Pool_Allocator_Context>();

		// Set the minimum chunk size to that of the machine word.
		if(chunk_size < sizeof(void *))
			chunk_size = sizeof(void *);

		self->ctx->arena      = arena_allocator_init(chunk_size * chunk_count);
		self->ctx->head       = nullptr;
		self->ctx->chunk_size = chunk_size;
	}

	Pool_Allocator::~Pool_Allocator()
	{
		Pool_Allocator *self = this;
		arena_allocator_deinit(self->ctx->arena);
		memory::deallocate(self->ctx);
	}

	Memory_Block
	Pool_Allocator::allocate(U64 size, U64 alignment)
	{
		Pool_Allocator *self = this;
		validate(size == 0 || size <= self->ctx->chunk_size, "[POOL_ALLOCATOR]: Requested allocation size exceeds pool chunk size.");
		validate(u64_is_power_of_two(alignment), "[POOL_ALLOCATOR]: Alignment must be a non-zero power of two.");
		if (alignment > alignof(void *))
		{
			// Pool chunks are allocated from the backing arena with pointer alignment.
			// Reused chunks keep that alignment; over-aligned chunks are not configured in this pool.
			log_fatal("[POOL_ALLOCATOR]: Requested alignment exceeds pool chunk alignment.");
		}

		if(self->ctx->head == nullptr)
		{
			Memory_Block block = arena_allocator_allocate(self->ctx->arena, self->ctx->chunk_size, alignof(void *));
			::memset(block.data, 0, self->ctx->chunk_size);
			return block;
		}

		Pool_Allocator_Node *result = self->ctx->head;
		self->ctx->head = self->ctx->head->next;
		::memset(result, 0, self->ctx->chunk_size);
		return Memory_Block{result, self->ctx->chunk_size};
	}

	void
	Pool_Allocator::deallocate(Memory_Block block)
	{
		if (block.data == nullptr)
			return;

		Pool_Allocator *self = this;

		#if DEBUG
		{
			Pool_Allocator_Node *node = self->ctx->head;
			while(node)
			{
				if (node == block.data)
				{
					log_error("[POOL_ALLOCATOR]: Double free of memory at address '{}'.", block.data);
					return;
				}
				node = node->next;
			}
		}
		#endif

		Pool_Allocator_Node *node = (Pool_Allocator_Node *)block.data;
		node->next = self->ctx->head;
		self->ctx->head = node;
	}

	Pool_Allocator *
	pool_allocator_init(U64 chunk_size, U64 chunk_count)
	{
		return allocate_and_call_constructor<Pool_Allocator>(chunk_size, chunk_count);
	}

	void
	pool_allocator_deinit(Pool_Allocator *self)
	{
		deallocate_and_call_destructor(self);
	}

	Memory_Block
	pool_allocator_allocate(Pool_Allocator *self)
	{
		return self->allocate();
	}

	void
	pool_allocator_deallocate(Pool_Allocator *self, Memory_Block block)
	{
		self->deallocate(block);
	}
}