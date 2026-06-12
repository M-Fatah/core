# Memory & Allocators

**Header:** `core/memory/memory.h`

All containers and most utilities accept a `memory::Allocator *`. The allocator API returns a `Memory_Block`, so ownership carries both the pointer and the allocation size.

---

## Allocator Interface

```cpp
namespace memory {
    struct Allocator {
        virtual ~Allocator() = default;
        virtual Memory_Block allocate(U64 size, U64 alignment) = 0;
        virtual void  deallocate(Memory_Block block) = 0;
        virtual void  clear() {}
    };
}
```

`Memory_Block{nullptr, 0}` is valid to deallocate. Alignment must be non-zero and a power of two.

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
Memory_Block block = memory::allocate(memory::heap_allocator(), 1024, alignof(U8));
memory::deallocate(memory::heap_allocator(), block);
```

### Temp Allocator

A per-thread arena intended to be cleared every frame or tick. Use it for short-lived strings and intermediate buffers. Do not store pointers from it across frames.

```cpp
String msg = format("Hello {}!", name, memory::temp_allocator());
// msg.data is valid until temp_allocator is cleared
```

### Arena Allocator

Bump-pointer allocator. `deallocate` is a no-op; memory is reclaimed all at once with `clear()` or `deinit`. Default capacity is 4 MB.

```cpp
#include <core/memory/arena_allocator.h>

auto *arena = memory::arena_allocator_init();

auto arr = array_init<int>(arena);
array_push(arr, 42);

memory::arena_allocator_clear(arena);
memory::arena_allocator_deinit(arena);
```

### Pool Allocator

Fixed-size chunk allocator. All chunks are the same size. O(1) alloc and dealloc.

```cpp
#include <core/memory/pool_allocator.h>

auto *pool = memory::pool_allocator_init(64, 256);

Memory_Block chunk = memory::pool_allocator_allocate(pool);
memory::pool_allocator_deallocate(pool, chunk);

memory::pool_allocator_deinit(pool);
```

### Virtual Allocator

Page-backed allocator implemented through the platform virtual-memory API.

```cpp
#include <core/memory/virtual_allocator.h>

auto *allocator = memory::virtual_allocator_init();
Memory_Block block = memory::virtual_allocator_allocate(allocator, 128 * 1024, alignof(U8));

memory::virtual_allocator_deallocate(allocator, block);
memory::virtual_allocator_deinit(allocator);
```

---

## Custom Allocator

Inherit from `memory::Allocator` and implement the `Memory_Block` contract.

```cpp
struct My_Allocator : memory::Allocator
{
    Memory_Block allocate(U64 size, U64 alignment) override;
    void  deallocate(Memory_Block block) override;
};
```