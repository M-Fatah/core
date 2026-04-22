#include "core/memory/arena_allocator.h"

#include "core/log.h"

// TODO: Remove and use logger.
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

	Arena_Allocator::Arena_Allocator(U64 initial_capacity, Allocator *allocator)
	{
		Arena_Allocator *self = this;
		self->ctx = memory::allocate_zeroed<Arena_Allocator_Context>(allocator);
		if (self->ctx == nullptr)
			log_fatal("[ARENA_ALLOCATOR]: Could not allocate memory for initialization.");

		self->ctx->allocator = allocator;

		self->ctx->head = (Arena_Allocator_Node *)memory::allocate(allocator, sizeof(Arena_Allocator_Node) + initial_capacity, alignof(Arena_Allocator_Node));
		if (self->ctx->head == nullptr)
			log_fatal("[ARENA_ALLOCATOR]: Could not allocate memory with given size {}.", sizeof(Arena_Allocator_Node) + initial_capacity);

		self->ctx->head->capacity = initial_capacity;
		self->ctx->head->used     = 0;
		self->ctx->head->next     = nullptr;
		self->ctx->used_size      = 0;
		self->ctx->peak_size      = 0;
	}

	Arena_Allocator::~Arena_Allocator()
	{
		Arena_Allocator *self = this;

	#if DEBUG
		// TODO: Use logger.
		// ::printf("[ARENA_ALLOCATOR]: %" PRIu64 " bytes used at exit, %" PRIu64 " bytes peak size.\n", self->ctx->used_size, self->ctx->peak_size);
	#endif

		auto node = self->ctx->head;
		while (node)
		{
			auto next = node->next;
			memory::deallocate(self->ctx->allocator, node);
			node = next;
		}

		memory::deallocate(self->ctx->allocator, self->ctx);
	}

	void *
	Arena_Allocator::allocate(U64 size, U64 alignment)
	{
		Arena_Allocator *self = this;

		// Bump-pointer with alignment: round the current position up to the next
		// multiple of `alignment`, then carve out `size` bytes.
		auto head_start   = (U64)(self->ctx->head + 1);
		auto cur_pos      = head_start + self->ctx->head->used;
		auto aligned_pos  = (cur_pos + (alignment - 1)) & ~((U64)alignment - 1);
		U64  padding      = (U64)(aligned_pos - cur_pos);
		U64  consumed     = padding + size;

		self->ctx->used_size += consumed;
		self->ctx->peak_size = self->ctx->used_size > self->ctx->peak_size ? self->ctx->used_size : self->ctx->peak_size;

		if (self->ctx->head->used + consumed <= self->ctx->head->capacity)
		{
			self->ctx->head->used += consumed;
			return (void *)aligned_pos;
		}
		else
		{
			// Worst-case alignment padding inside a brand-new node is (alignment - 1).
			// Ensure the new node has enough room for the aligned allocation.
			U64 min_capacity = size + alignment;
			U64 capacity     = min_capacity > self->ctx->head->capacity ? min_capacity : self->ctx->head->capacity;
			auto node = (Arena_Allocator_Node *)memory::allocate(self->ctx->allocator, sizeof(Arena_Allocator_Node) + capacity, alignof(Arena_Allocator_Node));
			if (node == nullptr)
				log_fatal("[ARENA_ALLOCATOR]: Could not allocate memory with given size {}.", size);

			auto new_payload_start = (U64)(node + 1);
			auto new_aligned_pos   = (new_payload_start + (alignment - 1)) & ~((U64)alignment - 1);
			U64  new_padding       = (U64)(new_aligned_pos - new_payload_start);

			node->capacity  = capacity;
			node->used      = new_padding + size;
			node->next      = self->ctx->head;
			self->ctx->head = node;

			return (void *)new_aligned_pos;
		}
	}

	void
	Arena_Allocator::deallocate(void *)
	{

	}

	void
	Arena_Allocator::clear()
	{
		Arena_Allocator *self = this;
		if (self->ctx->peak_size >= self->ctx->head->capacity)
		{
			auto node = self->ctx->head;
			while (node)
			{
				auto next = node->next;
				memory::deallocate(self->ctx->allocator, node);
				node = next;
			}

			self->ctx->head = (Arena_Allocator_Node *)memory::allocate(self->ctx->allocator, sizeof(Arena_Allocator_Node) + self->ctx->peak_size, alignof(Arena_Allocator_Node));
			if (self->ctx->head == nullptr)
				log_fatal("[ARENA_ALLOCATOR]: Could not allocate memory with given size {}.", sizeof(Arena_Allocator_Node) + self->ctx->peak_size);
			self->ctx->head->capacity = self->ctx->peak_size;
			self->ctx->head->used     = 0;
			self->ctx->head->next     = nullptr;
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

	void *
	arena_allocator_allocate(Arena_Allocator *self, U64 size, U64 alignment)
	{
		return self->allocate(size, alignment);
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