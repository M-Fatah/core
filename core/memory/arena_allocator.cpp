#include "core/memory/arena_allocator.h"

#include "core/log.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/platform/platform.h"

#include <stdlib.h>

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
		Arena_Allocator_Node *head;
		U64 used;
		U64 peak;
	};

	inline static Arena_Allocator_Node *
	_arena_allocator_node_init(U64 capacity)
	{
		validate(capacity <= U64_MAX - sizeof(Arena_Allocator_Node), "[ARENA_ALLOCATOR]: Arena node capacity is too large.");

		Memory_Block block = platform_virtual_memory_reserve(sizeof(Arena_Allocator_Node) + capacity);
		if (block.data == nullptr)
			log_fatal("[ARENA_ALLOCATOR]: Could not reserve memory with given size {}.", sizeof(Arena_Allocator_Node) + capacity);

		Memory_Block header_block = Memory_Block {
			.data = block.data,
			.size = platform_virtual_memory_page_align(sizeof(Arena_Allocator_Node))
		};
		if (!platform_virtual_memory_commit(header_block))
		{
			platform_virtual_memory_release(block);
			log_fatal("[ARENA_ALLOCATOR]: Could not commit arena node header.");
		}

		Arena_Allocator_Node *node = (Arena_Allocator_Node *)block.data;
		node->capacity = block.size - sizeof(Arena_Allocator_Node);
		node->used     = 0;
		node->next     = nullptr;
		return node;
	}

	inline static void
	_arena_allocator_node_deinit(Arena_Allocator_Node *node)
	{
		platform_virtual_memory_release(Memory_Block {
			.data = node,
			.size = sizeof(Arena_Allocator_Node) + node->capacity
		});
	}

	inline static void
	_arena_allocator_node_deinit_list(Arena_Allocator_Node *node)
	{
		while (node)
		{
			Arena_Allocator_Node *next = node->next;
			_arena_allocator_node_deinit(node);
			node = next;
		}
	}

	inline static void
	_arena_allocator_node_commit_to_used(Arena_Allocator_Node *node, U64 used)
	{
		Memory_Block block = Memory_Block {
			.data = node,
			.size = platform_virtual_memory_page_align(sizeof(Arena_Allocator_Node) + used)
		};
		if (!platform_virtual_memory_commit(block))
			log_fatal("[ARENA_ALLOCATOR]: Could not commit arena node memory.");
	}

	Arena_Allocator::Arena_Allocator(U64 initial_capacity)
	{
		Arena_Allocator *self = this;
		self->ctx = (Arena_Allocator_Context *)::malloc(sizeof(Arena_Allocator_Context));
		if (self->ctx == nullptr)
			log_fatal("[ARENA_ALLOCATOR]: Could not allocate memory for initialization.");

		self->ctx->head = _arena_allocator_node_init(initial_capacity);
		self->ctx->used = 0;
		self->ctx->peak = 0;
	}

	Arena_Allocator::~Arena_Allocator()
	{
		Arena_Allocator *self = this;
		_arena_allocator_node_deinit_list(self->ctx->head);
		::free(self->ctx);
	}

	Memory_Block
	Arena_Allocator::allocate(U64 size, U64 alignment)
	{
		if (size == 0)
			return Memory_Block{};

		validate(u64_is_power_of_two(alignment), "[ARENA_ALLOCATOR]: Alignment must be a non-zero power of two.");

		Arena_Allocator *self = this;
		Arena_Allocator_Node *node = self->ctx->head;

		U8 *payload_start    = (U8 *)(node + 1);
		U8 *aligned_position = (U8 *)u64_align_up((U64)(payload_start + node->used), alignment);
		U64 used             = (U64)(aligned_position - payload_start) + size;
		U64 used_delta       = used - node->used;

		if (used > node->capacity)
		{
			node             = _arena_allocator_node_init(u64_max(size + alignment, node->capacity));
			node->next       = self->ctx->head;
			self->ctx->head  = node;
			payload_start    = (U8 *)(node + 1);
			aligned_position = (U8 *)u64_align_up((U64)payload_start, alignment);
			used             = (U64)(aligned_position - payload_start) + size;
			used_delta       = used;
		}

		_arena_allocator_node_commit_to_used(node, used);
		node->used       = used;
		self->ctx->used += used_delta;
		self->ctx->peak  = u64_max(self->ctx->peak, self->ctx->used);
		return Memory_Block{.data = aligned_position, .size = size};
	}

	void
	Arena_Allocator::deallocate(Memory_Block)
	{

	}

	void
	Arena_Allocator::clear()
	{
		Arena_Allocator *self = this;
		if (self->ctx->peak > self->ctx->head->capacity)
		{
			_arena_allocator_node_deinit_list(self->ctx->head);
			self->ctx->head = _arena_allocator_node_init(self->ctx->peak);
			_arena_allocator_node_commit_to_used(self->ctx->head, self->ctx->peak);
		}

		self->ctx->head->used = 0;
		self->ctx->used       = 0;
	}

	Arena_Allocator *
	arena_allocator_init(U64 initial_capacity)
	{
		return allocate_and_call_constructor<Arena_Allocator>(initial_capacity);
	}

	void
	arena_allocator_deinit(Arena_Allocator *self)
	{
		deallocate_and_call_destructor(self);
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

	Arena_Allocator_Mark
	arena_allocator_mark(Arena_Allocator *self)
	{
		return Arena_Allocator_Mark {
			.allocator       = self,
			.head            = self->ctx->head,
			.head_used       = self->ctx->head->used,
			.arena_used      = self->ctx->used
		};
	}

	void
	arena_allocator_reset_to_mark(Arena_Allocator *self, Arena_Allocator_Mark mark)
	{
		validate(mark.allocator == self, "[ARENA_ALLOCATOR]: Mark belongs to a different arena.");
		validate(mark.arena_used <= self->ctx->used, "[ARENA_ALLOCATOR]: Mark is ahead of this arena state.");

		Arena_Allocator_Node *node = self->ctx->head;
		while (node != mark.head)
		{
			Arena_Allocator_Node *next = node->next;
			_arena_allocator_node_deinit(node);
			node = next;
		}

		self->ctx->head = mark.head;
		self->ctx->head->used = mark.head_used;
		self->ctx->used       = mark.arena_used;
	}

	U64
	arena_allocator_get_used(Arena_Allocator *self)
	{
		return self->ctx->used;
	}

	U64
	arena_allocator_get_peak(Arena_Allocator *self)
	{
		return self->ctx->peak;
	}
}