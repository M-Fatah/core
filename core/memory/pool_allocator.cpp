#include "core/memory/pool_allocator.h"

#include "core/log.h"
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
		u64 chunk_size;
	};

	Pool_Allocator::Pool_Allocator(u64 chunk_size, u64 chunk_count)
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

	void *
	Pool_Allocator::allocate(u64)
	{
		Pool_Allocator *self = this;
		if(self->ctx->head == nullptr)
		{
			void *result = arena_allocator_allocate(self->ctx->arena, self->ctx->chunk_size);
			::memset(result, 0, self->ctx->chunk_size);
			return result;
		}

		Pool_Allocator_Node *result = self->ctx->head;
		self->ctx->head = self->ctx->head->next;
		::memset(result, 0, self->ctx->chunk_size);
		return result;
	}

	void
	Pool_Allocator::deallocate(void *data)
	{
		if (data == nullptr)
			return;

		Pool_Allocator *self = this;

		#if DEBUG
		{
			Pool_Allocator_Node *node = self->ctx->head;
			while(node)
			{
				if (node == data)
				{
					log_error("[POOL_ALLOCATOR]: Double free of memory at address '{}'.", data);
					return;
				}
				node = node->next;
			}
		}
		#endif

		Pool_Allocator_Node *node = (Pool_Allocator_Node *)data;
		node->next = self->ctx->head;
		self->ctx->head = node;
	}

	Pool_Allocator *
	pool_allocator_init(u64 chunk_size, u64 chunk_count)
	{
		return allocate_and_call_constructor<Pool_Allocator>(chunk_size, chunk_count);
	}

	void
	pool_allocator_deinit(Pool_Allocator *self)
	{
		deallocate_and_call_destructor(self);
	}

	void *
	pool_allocator_allocate(Pool_Allocator *self)
	{
		return self->allocate();
	}

	void
	pool_allocator_deallocate(Pool_Allocator *self, void *data)
	{
		self->deallocate(data);
	}
}