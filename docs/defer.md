# Defer

**Header:** `core/defer.h`

`DEFER(code)` schedules `code` to run at the end of the current scope, regardless of how it exits. It is a scope-guard implemented as an RAII lambda wrapper.

---

## Usage

```cpp
#include <core/defer.h>
#include <core/containers/array.h>

void process()
{
    auto arr = array_init<int>();
    DEFER(array_deinit(arr));   // runs when process() returns

    array_push(arr, 1);
    array_push(arr, 2);
    // arr is freed here automatically
}
```

Multiple defers run in **reverse order** (last-in, first-out):

```cpp
FILE *f = fopen("data.bin", "rb");
DEFER(fclose(f));

auto *arena = memory::arena_allocator_init();
DEFER(memory::arena_allocator_deinit(arena));  // runs first on exit
```

---

## Notes

- Uses `__COUNTER__` internally — safe in unity builds.
- Captures by reference (`[&]`) — be cautious with loop variables.
- No heap allocation — the lambda is stored on the stack.
