#include "arena_allocator.h"

#include "core/logger.h"

// TODO: Remove and use logger.
#include <inttypes.h>

namespace memory
{
	struct Arena_Allocator_Node
	{
		u64 capacity;
		u64 used;
		Arena_Allocator_Node *next;
	};

	struct Arena_Allocator_Context
	{
		Allocator *allocator;
		u64 used_size;
		u64 peak_size;
		Arena_Allocator_Node *head;
	};

	Arena_Allocator::Arena_Allocator(u64 initial_capacity, Allocator *allocator)
	{
		Arena_Allocator *self = this;
		self->ctx = memory::allocate_zeroed<Arena_Allocator_Context>(allocator);
		if (self->ctx == nullptr)
			LOG_FATAL("[ARENA_ALLOCATOR]: Could not allocate memory for initialization.");

		self->ctx->allocator = allocator;

		self->ctx->head = (Arena_Allocator_Node *)memory::allocate(allocator, sizeof(Arena_Allocator_Node) + initial_capacity);
		if (self->ctx->head == nullptr)
			LOG_FATAL("[ARENA_ALLOCATOR]: Could not allocate memory with given size {}.", sizeof(Arena_Allocator_Node) + initial_capacity);

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
		::printf("[ARENA_ALLOCATOR]: %" PRIu64 " bytes used at exit, %" PRIu64 " bytes peak size.\n", self->ctx->used_size, self->ctx->peak_size);
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
	Arena_Allocator::allocate(u64 size)
	{
		Arena_Allocator *self = this;
		self->ctx->used_size += size;
		self->ctx->peak_size = self->ctx->used_size > self->ctx->peak_size ? self->ctx->used_size : self->ctx->peak_size;

		if (self->ctx->head->used + size <= self->ctx->head->capacity)
		{
			auto data = (u8 *)(self->ctx->head + 1) + self->ctx->head->used;
			self->ctx->head->used += size;
			return data;
		}
		else
		{
			auto capacity = size > self->ctx->head->capacity ? size : self->ctx->head->capacity;
			auto node = (Arena_Allocator_Node *)memory::allocate(self->ctx->allocator, sizeof(Arena_Allocator_Node) + capacity);
			if (node == nullptr)
				LOG_FATAL("[ARENA_ALLOCATOR]: Could not allocate memory with given size {}.", size);
			node->capacity  = capacity;
			node->used      = 0;
			node->next      = self->ctx->head;
			self->ctx->head = node;

			LOG_DEBUG("[ARENA_ALLOCATOR]: Allocated a new node with given capacity {}.", capacity);

			return node + 1;
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

			self->ctx->head = (Arena_Allocator_Node *)memory::allocate(self->ctx->allocator, sizeof(Arena_Allocator_Node) + self->ctx->peak_size);
			if (self->ctx->head == nullptr)
				LOG_FATAL("[ARENA_ALLOCATOR]: Could not allocate memory with given size {}.", sizeof(Arena_Allocator_Node) + self->ctx->peak_size);
			self->ctx->head->capacity = self->ctx->peak_size;
			self->ctx->head->used     = 0;
			self->ctx->head->next     = nullptr;
		}

		self->ctx->head->used = 0;
		self->ctx->used_size  = 0;
	}

	Arena_Allocator *
	arena_allocator_init(u64 initial_capacity, Allocator *allocator)
	{
		return allocate_and_call_constructor<Arena_Allocator>(allocator, initial_capacity, allocator);
	}

	void
	arena_allocator_deinit(Arena_Allocator *self)
	{
		deallocate_and_call_destructor(self->ctx->allocator, self);
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