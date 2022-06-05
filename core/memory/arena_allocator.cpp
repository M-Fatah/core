#include "arena_allocator.h"

#include "core/logger.h"

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
		u64 used_size;
		u64 peak_size;
		Arena_Allocator_Node *head;
	};

	Arena_Allocator::Arena_Allocator(u64 initial_capacity)
	{
		Arena_Allocator *self = this;
		self->ctx = memory::allocate_zeroed<Arena_Allocator_Context>();
		if (self->ctx == nullptr)
			LOG_FATAL("[ARENA_ALLOCATOR]: Could not allocate memory for initialization.");

		self->ctx->head = (Arena_Allocator_Node *)memory::allocate(sizeof(Arena_Allocator_Node) + initial_capacity);
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
		LOG_DEBUG(
			"[ARENA_ALLOCATOR]: {} bytes used at exit, {} bytes peak size.",
			self->ctx->used_size,
			self->ctx->peak_size
		);

		auto node = self->ctx->head;
		while (node)
		{
			auto next = node->next;
			memory::deallocate(node);
			node = next;
		}

		memory::deallocate(self->ctx);
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
			auto node = (Arena_Allocator_Node *)memory::allocate(sizeof(Arena_Allocator_Node) + capacity);
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
				memory::deallocate(node);
				node = next;
			}

			self->ctx->head = (Arena_Allocator_Node *)memory::allocate(sizeof(Arena_Allocator_Node) + self->ctx->peak_size);
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
	arena_allocator_init(u64 initial_capacity)
	{
		return allocate_and_call_constructor<Arena_Allocator>(initial_capacity);
	}

	void
	arena_allocator_deinit(Arena_Allocator *self)
	{
		deallocate_and_call_destructor(self);
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

	Allocator *
	temp_allocator()
	{
		static Arena_Allocator self;
		return &self;
	}
}