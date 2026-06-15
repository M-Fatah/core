# Memory & Allocators

**Header:** `core/memory/memory.h`

All containers and most utilities accept a `memory::Allocator *`. The allocator API returns a `Memory_Block`, so ownership carries both the pointer and the allocation size.

---

## Allocator Interface

```cpp
namespace memory {
    struct Allocator {
        virtual Memory_Block allocate(U64 size, U64 alignment) = 0;
        virtual void deallocate(Memory_Block block) = 0;
    };
}
```

`memory::allocate` and `memory::deallocate` are convenience wrappers over the allocator interface. `Memory_Block{nullptr, 0}` is valid to deallocate. Alignment must be non-zero and a power of two.

---

## Raw Blocks

Use raw `Memory_Block` allocation for byte buffers and multi-element allocations.

```cpp
memory::Allocator *allocator = memory::heap_allocator();

Memory_Block block = memory::allocate(allocator, sizeof(I32) * count, alignof(I32));
I32 *values = (I32 *)block.data;

memory::deallocate(allocator, block);
```

---

## Typed Helpers

Typed helpers allocate one object only.

```cpp
MyStruct *value = memory::allocate<MyStruct>(allocator);
memory::deallocate(allocator, value);

MyStruct *constructed = memory::allocate_and_call_constructor<MyStruct>(allocator, arg1, arg2);
memory::deallocate_and_call_destructor(allocator, constructed);
```

There is no `allocate<T>(count)` API. Multi-element ownership stays explicit through `Memory_Block`.

---

## Built-in Allocators

### Heap Allocator

Default allocator for containers and utilities.

```cpp
memory::Allocator *heap = memory::heap_allocator();
Memory_Block block = memory::allocate(heap, 1024, alignof(U8));
memory::deallocate(heap, block);
```

### Temp Allocator

A global arena intended to be cleared every frame or tick. Use it for short-lived strings and intermediate buffers. Do not store pointers from it across frames. `memory::temp_allocator()` returns `memory::Allocator *`. The global temp arena is embedded in Core's memory context and does not depend on the heap allocator, so it remains available during heap leak reporting.

```cpp
String msg = format("Hello {}!", name, memory::temp_allocator());
// msg.data is valid until temp_allocator is cleared

memory::temp_allocator_clear();
```

For scoped scratch work, use a temp mark.

```cpp
#include <core/memory/arena_allocator.h>

memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
DEFER(memory::temp_allocator_reset_to_mark(mark));

Memory_Block scratch = memory::allocate(memory::temp_allocator(), 1024, alignof(U8));
```

### Arena Allocator

Bump-pointer allocator. `deallocate` is a no-op; memory is reclaimed all at once with `clear()` or `deinit`. Default capacity is 1 GB. Arena nodes use platform virtual memory internally: they reserve their address range up front and commit pages on demand. User-created arena objects are allocated through the heap allocator, so forgotten `arena_allocator_deinit` calls are visible in heap leak reports.

```cpp
#include <core/memory/arena_allocator.h>

auto *arena = memory::arena_allocator_init();

auto arr = array_init<int>(arena);
array_push(arr, 42);

memory::Arena_Allocator_Mark mark = memory::arena_allocator_mark(arena);
Memory_Block scratch = memory::arena_allocator_allocate(arena, 1024, alignof(U8));
memory::arena_allocator_reset_to_mark(arena, mark);

memory::arena_allocator_clear(arena);
memory::arena_allocator_deinit(arena);
```

Marks reset the arena to a previous stack position and free newer arena nodes. Resetting to a mark invalidates marks taken after it. The reported peak remains a high-water mark.

### Pool Allocator

Fixed-size chunk allocator. All chunks are the same size. O(1) alloc and dealloc.

```cpp
#include <core/memory/pool_allocator.h>

auto *pool = memory::pool_allocator_init(64, 256);

Memory_Block chunk = memory::pool_allocator_allocate(pool);
memory::pool_allocator_deallocate(pool, chunk);

memory::pool_allocator_deinit(pool);
```

## Custom Allocator

Inherit from `memory::Allocator` and implement the `Memory_Block` allocation contract.

```cpp
struct My_Allocator : memory::Allocator
{
    Memory_Block allocate(U64 size, U64 alignment) override;
    void deallocate(Memory_Block block) override;
};
```