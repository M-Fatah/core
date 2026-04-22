# Memory & Allocators

**Header:** `core/memory/memory.h`

All containers and most utilities accept a `memory::Allocator *`. This makes the source of every allocation explicit and swappable.

---

## Allocator Interface

```cpp
namespace memory {
    struct Allocator {
        virtual void *allocate(u64 size) = 0;
        virtual void  deallocate(void *data) = 0;
        virtual void  clear() {}   // optional — only arena supports it
    };
}
```

---

## Built-in Allocators

### Heap Allocator

Wraps `malloc` / `free`. The default for all containers.

```cpp
memory::Allocator *alloc = memory::heap_allocator();
void *ptr = memory::allocate(alloc, 1024);
memory::deallocate(alloc, ptr);
```

### Temp (Scratch) Allocator

A per-thread arena that is intended to be cleared every frame / tick. Use it for short-lived strings and intermediate buffers. **Never store pointers from it across frames.**

```cpp
String msg = format("Hello {}!", name, memory::temp_allocator());
// msg.data is valid until temp_allocator is cleared
```

### Arena Allocator

Bump-pointer allocator. `deallocate` is a no-op — memory is reclaimed all at once with `clear()` or `deinit`. Default capacity is 4 MB.

```cpp
#include <core/memory/arena_allocator.h>

auto *arena = memory::arena_allocator_init();           // 4 MB default
// or: memory::arena_allocator_init(64 * 1024 * 1024); // 64 MB

auto arr = array_init<int>(arena);
array_push(arr, 42);

memory::arena_allocator_clear(arena);   // reclaim all at once
memory::arena_allocator_deinit(arena);  // free the arena itself
```

Key functions:

| Function | Description |
|---|---|
| `arena_allocator_init(capacity, backing)` | Create arena with given capacity |
| `arena_allocator_deinit(arena)` | Destroy arena |
| `arena_allocator_clear(arena)` | Reset offset to 0 (reuse memory) |
| `arena_allocator_get_used_size(arena)` | Bytes currently in use |
| `arena_allocator_get_peak_size(arena)` | Peak usage since last clear |

### Pool Allocator

Fixed-size chunk allocator. All chunks are the same size. O(1) alloc and dealloc.

```cpp
#include <core/memory/pool_allocator.h>

// Pool of 256 chunks, each 64 bytes
auto *pool = memory::pool_allocator_init(64, 256);

void *chunk = memory::pool_allocator_allocate(pool);
// ... use chunk ...
memory::pool_allocator_deallocate(pool, chunk);
memory::pool_allocator_deinit(pool);
```

---

## Typed helpers

`memory.h` provides typed wrappers for convenience:

```cpp
// Allocate sizeof(T) bytes
MyStruct *s = memory::allocate<MyStruct>(allocator);

// Allocate + call constructor
MyStruct *s = memory::allocate_and_call_constructor<MyStruct>(allocator, arg1, arg2);

// Allocate zeroed
void *p = memory::allocate_zeroed(allocator, size);
```

---

## Custom Allocator

Inherit from `memory::Allocator` and implement `allocate` / `deallocate`:

```cpp
struct My_Allocator : memory::Allocator
{
    void *allocate(u64 size) override { return my_malloc(size); }
    void  deallocate(void *data) override { my_free(data); }
};
```
