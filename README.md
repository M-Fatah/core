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
| `core/memory/` | Allocator interface, heap, arena, pool, and temp allocators |
| `core/containers/array.h` | Dynamic array (`Array<T>`) |
| `core/containers/stack_array.h` | Fixed-capacity stack array (`Stack_Array<T, N>`) |
| `core/containers/span.h` | Non-owning view (`Span<T>`) |
| `core/containers/string.h` | Heap-allocated string (`String`) |
| `core/containers/hash_table.h` | Open-addressing hash table (`Hash_Table<K, V>`) |
| `core/containers/hash_set.h` | Hash set (`Hash_Set<K>`) |
| `core/containers/string_interner.h` | String deduplication (`String_Interner`) |
| `core/formatter.h` | Type-safe string formatting (`format()`) |
| `core/print.h` / `core/log.h` | Colored output and log levels |
| `core/defer.h` | Scope-exit macro (`DEFER`) |
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
#include <core/defer.h>
#include <core/log.h>

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
};

auto vertices = array_init<Vertex>();
DEFER(array_deinit(vertices));

array_push(vertices, Vertex{1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f});
array_push(vertices, Vertex{4.0f, 5.0f, 6.0f, 0.0f, 1.0f, 0.0f});

log_info("loaded {} vertices", vertices.count);
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