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
