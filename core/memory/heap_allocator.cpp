#include "core/memory/heap_allocator.h"

#include "core/logger.h"
#include "core/platform/platform.h"

#include <stdlib.h>
#if DEBUG
#include <mutex>
#endif

namespace memory
{
#if DEBUG
	inline static constexpr u32 CALLSTACK_MAX_FRAME_COUNT = 20;
	struct Heap_Allocator_Node
	{
		u64 size;
		void *callstack[CALLSTACK_MAX_FRAME_COUNT];
		u32 callstack_frame_count;
		Heap_Allocator_Node *next;
		Heap_Allocator_Node *prev;
	};

	struct Heap_Allocator_Context
	{
		Heap_Allocator_Node *head;
		std::mutex mutex;
	};
#endif

	Heap_Allocator::Heap_Allocator()
	{
#if DEBUG
		Heap_Allocator *self = this;
		self->ctx = ::new Heap_Allocator_Context;
		if (self->ctx == nullptr)
			LOG_FATAL("[HEAP_ALLOCATOR]: Could not allocate memory for initialization.");
		self->ctx->head = nullptr;
#endif
	}

	Heap_Allocator::~Heap_Allocator()
	{
#if DEBUG
		Heap_Allocator *self = this;
		if (self->ctx->head == nullptr)
			return;

		u64 total_leak_count = 0;
		u64 total_leak_size  = 0;

		LOG_WARNING("memory leak detected:");
		LOG_WARNING("==================================================================");

		Heap_Allocator_Node *node = self->ctx->head;
		while (node)
		{
			LOG_WARNING("size: {} byte{}", node->size, node->size > 1 ? "s" : "");

			platform_callstack_log(node->callstack, node->callstack_frame_count);

			LOG_WARNING("==================================================================");

			++total_leak_count;
			total_leak_size += node->size;
			node = node->prev;
		}

		LOG_WARNING("total count: {}, total size: {} byte{}", total_leak_count, total_leak_size, total_leak_size > 1 ? "s" : "");

		::delete self->ctx;
#endif
	}

	void *
	Heap_Allocator::allocate(u64 size)
	{
#if DEBUG
		Heap_Allocator *self = this;

		Heap_Allocator_Node *node = (Heap_Allocator_Node *)::malloc(sizeof(Heap_Allocator_Node) + size);
		if (node == nullptr)
			LOG_FATAL("[HEAP_ALLOCATOR]: Could not allocate memory with given size {}.", size);

		if (node != nullptr && size != 0)
		{
			node->size = size;
			node->next = nullptr;

			self->ctx->mutex.lock();
			{
				node->prev = self->ctx->head;
				if (self->ctx->head != nullptr)
					self->ctx->head->next = node;
				self->ctx->head = node;
			}
			self->ctx->mutex.unlock();

			node->callstack_frame_count = platform_callstack_capture(node->callstack, CALLSTACK_MAX_FRAME_COUNT);

			return (node + 1);
		}

		return nullptr;
#else
		void *data = ::malloc(size);
		if (data == nullptr)
			LOG_FATAL("[HEAP_ALLOCATOR]: Could not allocate memory with given size {}.", size);
		return data;
#endif
	}

	void
	Heap_Allocator::deallocate(void *data)
	{
#if DEBUG
		Heap_Allocator *self = this;
		if (data != nullptr)
		{
			Heap_Allocator_Node *node = ((Heap_Allocator_Node *)data) - 1;

			self->ctx->mutex.lock();
			{
				if (node == self->ctx->head)
					self->ctx->head = node->prev;

				if (node->next)
					node->next->prev = node->prev;

				if (node->prev)
					node->prev->next = node->next;
			}
			self->ctx->mutex.unlock();

			::free(node);
		}
#else
		::free(data);
#endif
	}

	Heap_Allocator *
	heap_allocator_init()
	{
		return ::new Heap_Allocator;
	}

	void
	heap_allocator_deinit(Heap_Allocator *self)
	{
		::delete self;
	}

	void *
	heap_allocator_allocate(Heap_Allocator *self, u64 size)
	{
		return self->allocate(size);
	}

	void
	heap_allocator_deallocate(Heap_Allocator *self, void *data)
	{
		self->deallocate(data);
	}

	Allocator *
	heap_allocator()
	{
		static Heap_Allocator self;
		return &self;
	}
}