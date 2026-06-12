#include "core/memory/heap_allocator.h"

#include "core/log.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/platform/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#if COMPILER_MSVC
#include <malloc.h>
#endif
#if DEBUG
#include <mutex>
#endif

namespace memory
{
#if DEBUG
	inline static constexpr U32 CALLSTACK_MAX_FRAME_COUNT = 20;

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

	static void *
	_aligned_allocate(U64 size, U64 alignment)
	{
		U64 min_align = alignment;
		if (min_align < alignof(void *))
			min_align = alignof(void *);

		void *data = nullptr;
		#if COMPILER_MSVC
			data = ::_aligned_malloc(size, min_align);
		#else
			if (::posix_memalign(&data, min_align, size) != 0)
				data = nullptr;
		#endif
		return data;
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

		::printf("memory leak detected:\n");
		::printf("==================================================================\n");

		Heap_Allocator_Node *node = self->ctx->head;
		while (node)
		{
			::printf("size: %" PRIu64 " byte%s\n", node->size, node->size > 1 ? "s" : "");
			platform_callstack_log(node->callstack, node->callstack_frame_count);
			::printf("==================================================================\n");

			++total_leak_count;
			total_leak_size += node->size;
			node = node->prev;
		}

		::printf("total count: %" PRIu64 ", total size: %" PRIu64 "byte%s\n", total_leak_count, total_leak_size, total_leak_size > 1 ? "s" : "");

		::delete self->ctx;
#endif
	}

	Memory_Block
	Heap_Allocator::allocate(U64 size, U64 alignment)
	{
		if (size == 0)
			return Memory_Block{};

		validate(u64_is_power_of_two(alignment), "[HEAP_ALLOCATOR]: Alignment must be a non-zero power of two.");

		void *data = _aligned_allocate(size, alignment);
		if (data == nullptr)
			log_fatal("[HEAP_ALLOCATOR]: Could not allocate memory with size {} alignment {}.", size, alignment);

		#if DEBUG
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
		#endif

		return Memory_Block{data, size};
	}

	void
	Heap_Allocator::deallocate(Memory_Block block)
	{
		if (block.data == nullptr)
			return;

#if DEBUG
		Heap_Allocator *self = this;
		Heap_Allocator_Node *node = nullptr;
		self->ctx->mutex.lock();
		{
			for (Heap_Allocator_Node *it = self->ctx->head; it != nullptr; it = it->prev)
			{
				if (it->data == block.data)
				{
					node = it;
					break;
				}
			}

			validate(node != nullptr, "[HEAP_ALLOCATOR]: Tried to deallocate a block that was not allocated by this allocator.");
			validate(node->size == block.size, "[HEAP_ALLOCATOR]: Deallocated block size does not match allocated block size.");

			if (node == self->ctx->head)
				self->ctx->head = node->prev;

			if (node->next)
				node->next->prev = node->prev;

			if (node->prev)
				node->prev->next = node->next;
		}
		self->ctx->mutex.unlock();

		::free(node);
#endif

		#if COMPILER_MSVC
			::_aligned_free(block.data);
		#else
			::free(block.data);
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

	Memory_Block
	heap_allocator_allocate(Heap_Allocator *self, U64 size, U64 alignment)
	{
		return self->allocate(size, alignment);
	}

	void
	heap_allocator_deallocate(Heap_Allocator *self, Memory_Block block)
	{
		self->deallocate(block);
	}
}