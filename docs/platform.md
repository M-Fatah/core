# Platform

**Header:** `core/platform/platform.h`

Cross-platform file I/O, path utilities, and native dialogs. Implementations provided for Windows, Linux, and macOS.

---

## File I/O

```cpp
#include <core/platform/platform.h>

// Read entire file into a String
String contents = platform_file_read("data/config.json");
DEFER(string_deinit(contents));

// With explicit allocator
String contents = platform_file_read("shader.glsl", memory::temp_allocator());

// Read/write into caller-owned memory
Memory_Block block = memory::allocate(file_size, alignof(U8));
DEFER(memory::deallocate(block));

U64 bytes_read = platform_file_read("data.bin", block);
U64 bytes_written = platform_file_write("copy.bin", block);
```

Raw file read/write APIs take `Memory_Block`; higher-level path helpers still return `String` for whole-file text-style reads.

---

## Virtual Memory

Platform exposes page-based virtual-memory primitives. Allocator policy lives in `core/memory/virtual_allocator.h`.

```cpp
U64 page_size = platform_virtual_memory_get_page_size();
Memory_Block block = platform_virtual_memory_reserve(page_size);

platform_virtual_memory_commit(block);
platform_virtual_memory_decommit(block);
platform_virtual_memory_release(block);
```

---

## Path Utilities

```cpp
bool valid = platform_path_is_valid("assets/textures");
bool is_file = platform_path_is_file("assets/albedo.png");
bool is_dir  = platform_path_is_directory("assets/");
```

All functions accept both `String` and `const char *`.

---

## Dialogs

```cpp
// Open file picker — returns chosen path or empty string
String path = platform_dialog_file_open("Open Scene", "*.scene");
DEFER(string_deinit(path));

if (!string_is_empty(path))
    load_scene(path);
```

```cpp
// Save file picker
String path = platform_dialog_file_save("Save Scene", "*.scene");
```

---

## Platform Macros

The build system defines these so you can conditionally compile:

| Macro | Platform |
|---|---|
| `PLATFORM_WINDOWS` | Windows |
| `PLATFORM_LINUX` | Linux |
| `PLATFORM_MACOS` | macOS |
| `COMPILER_MSVC` | MSVC |
| `COMPILER_CLANG` | Clang / Apple Clang |
| `COMPILER_GCC` | GCC |
| `DEBUG` | Debug / RelWithDebInfo builds |
| `RELEASE` | Release / MinSizeRel builds |
