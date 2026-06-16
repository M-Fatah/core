#include "core/memory/heap_allocator.h"

#include "core/log.h"
#include "core/validate.h"
#include "core/math/u64.h"
#include "core/platform/platform.h"

#include <stdlib.h>
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


	inline static void
	_heap_allocator_track_allocation(Heap_Allocator *self, void *data, U64 size)
	{
		constexpr auto _node_init = [](void *data, U64 size) -> Heap_Allocator_Node * {
			Heap_Allocator_Node *node = (Heap_Allocator_Node *)::malloc(sizeof(Heap_Allocator_Node));
			if (node == nullptr)
				log_fatal("[HEAP_ALLOCATOR]: Could not allocate debug tracking node.");

			node->size                  = size;
			node->data                  = data;
			node->next                  = nullptr;
			node->prev                  = nullptr;
			node->callstack_frame_count = platform_callstack_capture(node->callstack, CALLSTACK_MAX_FRAME_COUNT);
			return node;
		};

		Heap_Allocator_Node *node = _node_init(data, size);

		self->ctx->mutex.lock();
		{
			node->prev = self->ctx->head;
			if (self->ctx->head != nullptr)
				self->ctx->head->next = node;
			self->ctx->head = node;
		}
		self->ctx->mutex.unlock();
	}

	inline static Heap_Allocator_Node *
	_heap_allocator_untrack_allocation(Heap_Allocator *self, Memory_Block block)
	{
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

		return node;
	}
#endif

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

			log_warning("Memory leak detected:");
			log_warning("==================================================================");

			Heap_Allocator_Node *node = self->ctx->head;
			while (node)
			{
				log_warning("size: {} byte{}", node->size, node->size > 1 ? "s" : "");
				platform_callstack_log(node->callstack, node->callstack_frame_count);
				log_warning("==================================================================");

				++total_leak_count;
				total_leak_size += node->size;
				node = node->prev;
			}

			log_warning("Total count = {} and size = {} byte{}", total_leak_count, total_leak_size, total_leak_size > 1 ? "s" : "");

			::delete self->ctx;
		#endif
	}

	Memory_Block
	Heap_Allocator::allocate(U64 size, U64 alignment)
	{
		constexpr auto _aligned_allocate = [](U64 size, U64 alignment) -> void * {
			#if COMPILER_MSVC
				return ::_aligned_malloc(size, u64_max(alignment, alignof(void *)));
			#else
				void *data = nullptr;
				if (::posix_memalign(&data, u64_max(alignment, alignof(void *)), size) != 0)
					data = nullptr;
				return data;
			#endif
		};

		if (size == 0)
			return Memory_Block{};

		validate(u64_is_power_of_two(alignment), "[HEAP_ALLOCATOR]: Alignment must be a non-zero power of two.");

		void *data = _aligned_allocate(size, alignment);
		if (data == nullptr)
			log_fatal("[HEAP_ALLOCATOR]: Could not allocate memory with size {} alignment {}.", size, alignment);

		#if DEBUG
			_heap_allocator_track_allocation(this, data, size);
		#endif

		return Memory_Block{data, size};
	}

	void
	Heap_Allocator::deallocate(Memory_Block block)
	{
		constexpr auto _aligned_deallocate = [](void *data) -> void {
			#if COMPILER_MSVC
				::_aligned_free(data);
			#else
				::free(data);
			#endif
		};

		if (block.data == nullptr)
			return;

		#if DEBUG
			Heap_Allocator_Node *node = _heap_allocator_untrack_allocation(this, block);
			::free(node);
		#endif

		_aligned_deallocate(block.data);
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