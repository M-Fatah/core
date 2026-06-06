# **Core**
<!-- badges: start -->
[![Build status](https://github.com/M-Fatah/core/workflows/CI/badge.svg)](https://github.com/M-Fatah/core/actions?workflow=CI)
![Lines of code](https://img.shields.io/tokei/lines/github/M-Fatah/core)
<!-- badges: end -->

---

## **Introduction**

**Core** is a C-like C++20 library providing foundational utilities for data-oriented programming. It is designed as a replacement for the STL with a simpler, more explicit design:

- **Explicit allocations** — every container accepts an `Allocator *`, making every allocation visible and swappable (heap, arena, pool, temp).
- **No exceptions** — errors are returned as `Error` values.
- **Explicit lifetimes** — `_init` / `_deinit` pairs make every allocation and ownership transfer visible at the call site.
- **C-like style** — free functions over methods, plain structs over class hierarchies.

> This library is still a WIP — breaking changes are expected.

---

## **Modules**

| Module | Description |
|---|---|
| `core/defines.h` | Primitive aliases (`I8`..`I64`, `U8`..`U64`, `F32`, `F64`) and macro utilities |
| `core/memory/` | Allocator interface, heap, arena, pool, and temp allocators (with alignment) |
| `core/containers/array.h` | Dynamic array (`Array<T>`) |
| `core/containers/stack_array.h` | Fixed-capacity stack array (`Stack_Array<T, N>`) |
| `core/containers/span.h` | Non-owning view (`Span<T>`) |
| `core/containers/string.h` | Heap-allocated string (`String`) |
| `core/containers/hash_table.h` | Open-addressing hash table (`Hash_Table<K, V>`) |
| `core/containers/hash_set.h` | Hash set (`Hash_Set<K>`) |
| `core/containers/string_interner.h` | String deduplication (`String_Interner`) |
| `core/math/` | Vectors, matrices, quaternion, scalar helpers, random — NEON / AVX / scalar |
| `core/formatter.h` | Type-safe string formatting (`format()`) — math types format natively |
| `core/print.h` / `core/log.h` | Colored output and log levels |
| `core/defer.h` | Scope-exit macro (`DEFER`) |
| `core/tester.h` | Minimal unit-test framework (`TESTER_TEST`, `TESTER_CHECK`) |
| `core/validate.h` | Runtime assertions with source location |
| `core/result.h` | Error-returning pattern (`Error`, `Result<T>`) |
| `core/hash.h` | FNV-32 and type-generic `hash()` |
| `core/reflect.h` | Compile-time type reflection |
| `core/serialization/` | Binary and JSON serializers |
| `core/ecs.h` | Minimal entity-component system |
| `core/platform/platform.h` | File I/O, path utilities, native dialogs |

📖 **Full documentation is in the [`docs/`](docs/home.md) folder.**

---

## **Quick Example**

```cpp
#include <core/containers/array.h>
#include <core/math/f32x3.h>
#include <core/math/f32x4x4.h>
#include <core/defer.h>
#include <core/log.h>

struct Vertex
{
    F32x3 position;
    F32x3 normal;
};

auto vertices = array_init<Vertex>();
DEFER(array_deinit(vertices));

array_push(vertices, Vertex{F32x3{1.0f, 2.0f, 3.0f}, F32X3_UP});
array_push(vertices, Vertex{F32x3{4.0f, 5.0f, 6.0f}, F32X3_FORWARD});

F32x4x4 view = f32x4x4_look_at(F32x3{0, 0, 5}, F32X3_ZERO, F32X3_UP);
log_info("loaded {} vertices, view matrix:\n{}", vertices.count, view);
```

---

## **Platforms**

| Platform | Status |
|---|---|
| Windows | ✅ |
| Linux | ✅ |
| macOS | ✅ |

---

## **Prerequisites**

#### **Windows**
- [CMake](https://cmake.org/download/) 3.20+

#### **Linux**
```bash
sudo apt update
sudo apt-get install -y cmake libx11-dev libxkbcommon-x11-dev libx11-xcb-dev zenity
```

#### **macOS**
- Xcode Command Line Tools: `xcode-select --install`
- [CMake](https://cmake.org/download/) 3.20+

---

## **Building**

```bash
cmake -B build
cmake --build build --config Debug -j
```

Output is placed in `build/bin/Debug/`.

### Options

| CMake Option | Default | Description |
|---|---|---|
| `CORE_BUILD_UNITTEST` | ON (main project) | Build unit tests |
| `CORE_INSTALL` | ON (main project) | Enable install target |
| `CORE_BUILD_UNITY` | OFF | Enable unity (single-TU) build |
| `CORE_BUILD_STATIC` | OFF | Build as a static library |

### Running Tests

```bash
cmake -B build
cmake --build build --config Debug
./build/bin/Debug/unittest
```