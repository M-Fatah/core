# Core — Documentation

**Core** is a C-like C++20 library providing foundational utilities for data-oriented programming. It replaces the STL with a simpler, more explicit set of containers, allocators, and utilities.

---

## Modules

| Module | Header | Description |
|---|---|---|
| [Memory & Allocators](memory.md) | `core/memory/memory.h` | Allocator interface, heap, arena, pool, temp allocators |
| [Containers](containers.md) | `core/containers/` | Array, Stack\_Array, Span, String, Hash\_Table, Hash\_Set, String\_Interner |
| [Formatter](formatter.md) | `core/formatter.h` | `format()` / `Formatter` — type-safe string formatting |
| [Print & Log](print-log.md) | `core/print.h`, `core/log.h` | Colored output, log levels |
| [Defer](defer.md) | `core/defer.h` | RAII scope-exit macro |
| [Validate](validate.md) | `core/validate.h` | Runtime assertions with source location |
| [Result & Error](result.md) | `core/result.h` | Error-returning pattern without exceptions |
| [Hash](hash.md) | `core/hash.h` | FNV-32 and type-generic `hash()` overloads |
| [Reflect](reflect.md) | `core/reflect.h` | Compile-time type reflection: kinds, names, fields, enums |
| [Serialization](serialization.md) | `core/serialization/` | Binary and JSON serializers |
| [ECS](ecs.md) | `core/ecs.h` | Minimal entity-component system |
| [Platform](platform.md) | `core/platform/platform.h` | File I/O, paths, dialogs |

---

## Quick Start

```cmake
# CMakeLists.txt
add_subdirectory(core)
target_link_libraries(my_app PRIVATE core)
```

```cpp
#include <core/containers/array.h>
#include <core/defer.h>

auto arr = array_init<int>();
DEFER(array_deinit(arr));

array_push(arr, 1);
array_push(arr, 2);
array_push(arr, 3);

for (int x : arr)
    print_to(stdout, "{}\n", x);
```

---

## Design Philosophy

- **C-like style** — free functions over methods, explicit over implicit.
- **No hidden allocations** — every container takes an `Allocator *`.
- **No exceptions** — errors are returned as `Error` values.
- **No RTTI** — compile-time reflection via `reflect.h`.
- **Explicit lifetimes** — `_init` / `_deinit` pairs, `DEFER` for cleanup.
