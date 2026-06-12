#include "core/memory/arena_allocator.h"

#include "core/log.h"
#include "core/validate.h"
#include "core/math/u64.h"

#include <stdio.h>
#include <inttypes.h>

namespace memory
{
	struct Arena_Allocator_Node
	{
		U64 capacity;
		U64 used;
		Arena_Allocator_Node *next;
	};

	struct Arena_Allocator_Context
	{
		Allocator *allocator;
		U64 used_size;
		U64 peak_size;
		Arena_Allocator_Node *head;
	};

	inline static Arena_Allocator_Node *
	_arena_allocator_node_init(U64 capacity, Allocator *allocator)
	{
		Memory_Block block = memory::allocate(allocator, sizeof(Arena_Allocator_Node) + capacity, alignof(Arena_Allocator_Node));
		if (block.data == nullptr)
			log_fatal("[ARENA_ALLOCATOR]: Could not allocate memory with given size {}.", block.size);

		Arena_Allocator_Node *node = (Arena_Allocator_Node *)block.data;
		node->capacity = capacity;
		node->used     = 0;
		node->next     = nullptr;
		return node;
	}

	Arena_Allocator::Arena_Allocator(U64 initial_capacity, Allocator *allocator)
	{
		Arena_Allocator *self = this;
		self->ctx = memory::allocate_zeroed<Arena_Allocator_Context>(allocator);
		if (self->ctx == nullptr)
			log_fatal("[ARENA_ALLOCATOR]: Could not allocate memory for initialization.");

		self->ctx->allocator = allocator;
		self->ctx->head      = _arena_allocator_node_init(initial_capacity, allocator);
		self->ctx->used_size = 0;
		self->ctx->peak_size = 0;
	}

	Arena_Allocator::~Arena_Allocator()
	{
		Arena_Allocator *self = this;
		Arena_Allocator_Node *node = self->ctx->head;
		while (node)
		{
			Arena_Allocator_Node *next = node->next;
			memory::deallocate(self->ctx->allocator, Memory_Block{node, sizeof(Arena_Allocator_Node) + node->capacity});
			node = next;
		}
		memory::deallocate(self->ctx->allocator, self->ctx);
	}

	Memory_Block
	Arena_Allocator::allocate(U64 size, U64 alignment)
	{
		if (size == 0)
			return Memory_Block{};

		validate(u64_is_power_of_two(alignment), "[ARENA_ALLOCATOR]: Alignment must be a non-zero power of two.");

		Arena_Allocator *self = this;

		U8 *payload_start    = (U8 *)(self->ctx->head + 1);
		U8 *aligned_position = (U8 *)u64_align_up((U64)(payload_start + self->ctx->head->used), alignment);
		U64 used             = (U64)(aligned_position - payload_start) + size;
		if (used <= self->ctx->head->capacity)
		{
			self->ctx->used_size += used - self->ctx->head->used;
			self->ctx->peak_size  = u64_max(self->ctx->peak_size, self->ctx->used_size);
			self->ctx->head->used = used;
			return Memory_Block{.data = aligned_position, .size = size};
		}
		else
		{
			Arena_Allocator_Node *node = _arena_allocator_node_init(u64_max(size + alignment, self->ctx->head->capacity), self->ctx->allocator);
			payload_start              = (U8 *)(node + 1);
			aligned_position           = (U8 *)u64_align_up((U64)payload_start, alignment);
			used                       = (U64)(aligned_position - payload_start) + size;
			node->used                 = used;
			node->next                 = self->ctx->head;
			self->ctx->head            = node;
			self->ctx->used_size      += used;
			self->ctx->peak_size       = u64_max(self->ctx->peak_size, self->ctx->used_size);
			return Memory_Block{.data = aligned_position, .size = size};
		}
	}

	void
	Arena_Allocator::deallocate(Memory_Block)
	{

	}

	void
	Arena_Allocator::clear()
	{
		Arena_Allocator *self = this;
		if (self->ctx->peak_size > self->ctx->head->capacity)
		{
			Arena_Allocator_Node *node = self->ctx->head;
			while (node)
			{
				Arena_Allocator_Node *next = node->next;
				memory::deallocate(self->ctx->allocator, Memory_Block{node, sizeof(Arena_Allocator_Node) + node->capacity});
				node = next;
			}

			self->ctx->head = _arena_allocator_node_init(self->ctx->peak_size, self->ctx->allocator);
		}

		self->ctx->head->used = 0;
		self->ctx->used_size  = 0;
	}

	Arena_Allocator *
	arena_allocator_init(U64 initial_capacity, Allocator *allocator)
	{
		return allocate_and_call_constructor<Arena_Allocator>(allocator, initial_capacity, allocator);
	}

	void
	arena_allocator_deinit(Arena_Allocator *self)
	{
		deallocate_and_call_destructor(self->ctx->allocator, self);
	}

	Memory_Block
	arena_allocator_allocate(Arena_Allocator *self, U64 size, U64 alignment)
	{
		return self->allocate(size, alignment);
	}

	void
	arena_allocator_deallocate(Arena_Allocator *self, Memory_Block block)
	{
		self->deallocate(block);
	}

	void
	arena_allocator_clear(Arena_Allocator *self)
	{
		self->clear();
	}

	U64
	arena_allocator_get_used_size(Arena_Allocator *self)
	{
		return self->ctx->used_size;
	}

	U64
	arena_allocator_get_peak_size(Arena_Allocator *self)
	{
		return self->ctx->peak_size;
	}
}