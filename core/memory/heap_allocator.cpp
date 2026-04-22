#include "core/memory/heap_allocator.h"

#include "core/log.h"
#include "core/platform/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#if DEBUG
#include <mutex>
#endif

namespace memory
{
#if DEBUG
	inline static constexpr U32 CALLSTACK_MAX_FRAME_COUNT = 20;

	// In DEBUG we track each live allocation in a linked list. The node lives in its
	// own ::malloc (independent from the user's aligned payload) and is reachable O(1)
	// from the user pointer via a back-pointer slot placed just before the aligned data.
	struct Heap_Allocator_Node
	{
		U64 size;
		void *data;
		void *callstack[CALLSTACK_MAX_FRAME_COUNT];
		U32 callstack_frame_count;
		Heap_Allocator_Node *next;
		Heap_Allocator_Node *prev;
	};

	struct Heap_Allocator_Context
	{
		Heap_Allocator_Node *head;
		std::mutex mutex;
	};
#endif

	// Allocate aligned memory via plain malloc, reserving extra slot(s) before the returned
	// pointer for bookkeeping. Layout:
	//
	//   [ ... wasted padding ... ][ slot[-N] ... slot[-1] ][ user data aligned ]
	//                             ^ header_slots * sizeof(void*) before user data
	//
	// slot[-1] always stores the raw ::malloc base so deallocate() can free it.
	// slot[-2] (when reserved) stores the debug Heap_Allocator_Node pointer.
	//
	// This works with any alignment >= alignof(void*) and avoids the _aligned_malloc vs
	// posix_memalign vs free/_aligned_free platform fork — single ::malloc/::free path.
	static void *
	_aligned_malloc_with_slots(U64 size, U64 alignment, U64 extra_slots_before)
	{
		if (size == 0)
			return nullptr;

		U64 min_align = alignment;
		if (min_align < alignof(void *))
			min_align = alignof(void *);

		U64 header_bytes = sizeof(void *) * (extra_slots_before + 1);
		U64 total = size + header_bytes + (min_align - 1);

		void *raw = ::malloc(total);
		if (raw == nullptr)
			return nullptr;

		U64 raw_addr = (U64)raw;
		U64 user_addr = (raw_addr + header_bytes + (min_align - 1)) & ~((U64)min_align - 1);

		void **slots = (void **)user_addr;
		slots[-1] = raw;

		return (void *)user_addr;
	}

	Heap_Allocator::Heap_Allocator()
	{
#if DEBUG
		Heap_Allocator *self = this;
		self->ctx = ::new Heap_Allocator_Context;
		if (self->ctx == nullptr)
			log_fatal("[HEAP_ALLOCATOR]: Could not allocate memory for initialization.");
		self->ctx->head = nullptr;
#endif
	}

	Heap_Allocator::~Heap_Allocator()
	{
#if DEBUG
		Heap_Allocator *self = this;
		if (self->ctx->head == nullptr)
		{
			::delete self->ctx;
			return;
		}

		U64 total_leak_count = 0;
		U64 total_leak_size  = 0;

		// TODO: Use logger.
		::printf("memory leak detected:\n");
		::printf("==================================================================\n");

		Heap_Allocator_Node *node = self->ctx->head;
		while (node)
		{
			// TODO: Use logger.
			::printf("size: %" PRIu64 " byte%s\n", node->size, node->size > 1 ? "s" : "");

			platform_callstack_log(node->callstack, node->callstack_frame_count);

			// TODO: Use logger.
			::printf("==================================================================\n");

			++total_leak_count;
			total_leak_size += node->size;
			node = node->prev;
		}

		// TODO: Use logger.
		::printf("total count: %" PRIu64 ", total size: %" PRIu64 "byte%s\n", total_leak_count, total_leak_size, total_leak_size > 1 ? "s" : "");

		::delete self->ctx;
#endif
	}

	void *
	Heap_Allocator::allocate(U64 size, U64 alignment)
	{
		if (size == 0)
			return nullptr;

#if DEBUG
		// Reserve 2 slots before user data: slot[-1]=raw base, slot[-2]=debug node pointer.
		void *data = _aligned_malloc_with_slots(size, alignment, /* extra_slots_before */ 1);
		if (data == nullptr)
			log_fatal("[HEAP_ALLOCATOR]: Could not allocate memory with size {} alignment {}.", size, alignment);

		Heap_Allocator *self = this;

		Heap_Allocator_Node *node = (Heap_Allocator_Node *)::malloc(sizeof(Heap_Allocator_Node));
		if (node == nullptr)
			log_fatal("[HEAP_ALLOCATOR]: Could not allocate debug tracking node.");

		node->size = size;
		node->data = data;
		node->next = nullptr;
		node->callstack_frame_count = platform_callstack_capture(node->callstack, CALLSTACK_MAX_FRAME_COUNT);

		self->ctx->mutex.lock();
		{
			node->prev = self->ctx->head;
			if (self->ctx->head != nullptr)
				self->ctx->head->next = node;
			self->ctx->head = node;
		}
		self->ctx->mutex.unlock();

		((void **)data)[-2] = node;

		return data;
#else
		// Release: only the raw-base back-pointer slot, no debug node.
		void *data = _aligned_malloc_with_slots(size, alignment, /* extra_slots_before */ 0);
		if (data == nullptr)
			log_fatal("[HEAP_ALLOCATOR]: Could not allocate memory with size {} alignment {}.", size, alignment);
		return data;
#endif
	}

	void
	Heap_Allocator::deallocate(void *data)
	{
		if (data == nullptr)
			return;

		void **slots = (void **)data;
		void *raw = slots[-1];

#if DEBUG
		Heap_Allocator *self = this;
		Heap_Allocator_Node *node = (Heap_Allocator_Node *)slots[-2];
		if (node != nullptr)
		{
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
#endif

		::free(raw);
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
	heap_allocator_allocate(Heap_Allocator *self, U64 size, U64 alignment)
	{
		return self->allocate(size, alignment);
	}

	void
	heap_allocator_deallocate(Heap_Allocator *self, void *data)
	{
		self->deallocate(data);
	}
}
