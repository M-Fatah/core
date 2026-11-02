# Memory & Allocators

**Header:** `core/memory/allocator.h`

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

## Copying Elements

Both `memory::copy` overloads count elements of `T`. They accept writable, trivially copyable element types, copy their representation without calling constructors, and do not duplicate pointed-to data or append a terminator. Core exposes `is_trivially_copyable_v<T>` in `core/defines.h`; const types can satisfy that trait, so these overloads separately reject const and volatile element types in their signatures.

```cpp
const U32 source[] = {10, 20, 30};
U32 destination[3];
memory::copy(destination, source, count_of(source));

memory::Allocator *allocator = memory::heap_allocator();
U32 *copied = memory::copy(allocator, source, count_of(source));
DEFER(memory::deallocate(allocator, Memory_Block {copied, sizeof(source)}));
```

- `copy(T *destination, const T *source, U64 count)` writes into existing storage. Both ranges must contain at least `count` elements and must not overlap. Zero elements is a no-op and permits null pointers.
- `copy(Allocator *allocator, const T *source, U64 count)` allocates `count * sizeof(T)` bytes with `alignof(T)` and copies the source. Zero elements returns `nullptr` without calling the allocator. If the allocator returns no storage, this overload returns `nullptr`; Core's heap and arena allocators terminate on allocation failure.

The caller must supply a valid source range, and `count * sizeof(T)` must fit the allocation size type. Keep the count for deallocation: `memory::deallocate(T *)` describes only one object. Use `Memory_Block {data, count * sizeof(T)}` for multiple elements, or `array_from(view, allocator)` when an owned array is the appropriate result.

For byte ranges, use `U8` pointers; an element then occupies one byte. Cast untyped addresses explicitly:

```cpp
memory::copy((U8 *)destination_address, (const U8 *)source_address, byte_count);
```

There is no `void *` overload. When migrating a byte-count call with wider element pointers, convert the count to elements or cast the pointers to `U8`. Passing `sizeof(source)` with `U32 *` pointers would request that many **U32 elements**.

The string interner still allocates its extra character and writes the terminator explicitly:

```cpp
memory::copy(data, text.data, text.count);
data[text.count] = '\0';
```

---

## Typed Helpers

`allocate<T>`, `allocate_zeroed<T>` and the constructor helpers allocate one object only.

```cpp
MyStruct *value = memory::allocate<MyStruct>(allocator);
memory::deallocate(allocator, value);

MyStruct *constructed = memory::allocate_and_call_constructor<MyStruct>(allocator, arg1, arg2);
memory::deallocate_and_call_destructor(allocator, constructed);
```

There is no `allocate<T>(count)` API. The allocating `copy` overload supports multiple elements; ownership still requires the full `Memory_Block` size when deallocating.

---

## Built-in Allocators

### Heap Allocator

Default allocator for containers and utilities. In debug builds it tracks live allocations and reports leaks on shutdown. Callstack capture/resolve comes from the platform API, but heap owns the leak-report formatting and logging.

```cpp
memory::Allocator *heap = memory::heap_allocator();
Memory_Block block = memory::allocate(heap, 1024, alignof(U8));
memory::deallocate(heap, block);
```

### Temp Allocator

A thread-local arena intended to be cleared every frame or tick on the calling thread. Use it for short-lived strings and intermediate buffers. Do not store pointers from it across frames or pass temp marks across threads. `memory::temp_allocator()` returns `memory::Allocator *`. The temp arena does not depend on the heap allocator, so it remains available during heap leak reporting.

```cpp
String msg = format("Hello {}!", name, memory::temp_allocator());
// msg.data is valid until this thread's temp allocator is cleared

memory::temp_allocator_clear();
```

For scoped scratch work, use a temp mark.

```cpp
#include <core/memory/arena_allocator.h>

memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
DEFER(memory::temp_allocator_reset_to_mark(mark));

Memory_Block scratch = memory::allocate(memory::temp_allocator(), 1024, alignof(U8));
```

Resetting to a temp mark invalidates scratch allocations made after that mark on the same thread and lets later temp allocations reuse that space.

### Arena Allocator

Bump-pointer allocator. `deallocate` is a no-op; memory is reclaimed all at once with `clear()` or `deinit`. Default capacity is 1 GB. Arena nodes use platform virtual memory internally: they reserve their address range up front and commit pages on demand. `clear()` is a fast reset: it keeps committed memory and sets used memory to zero. If the recorded peak outgrows the current head capacity, `clear()` releases all nodes and creates one committed node large enough for the peak. User-created arena objects are allocated through the heap allocator, so forgotten `arena_allocator_deinit` calls are visible in heap leak reports.

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

Marks reset the arena to a previous stack position and free newer arena nodes. The retained node keeps its committed memory. Resetting to a mark invalidates marks taken after it. The reported peak remains a high-water mark.

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