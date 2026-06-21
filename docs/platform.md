# Platform

**Header:** `core/platform/platform.h`

Cross-platform file I/O, path utilities, native dialogs, windows, and low-level platform primitives. Implementations provided for Windows, Linux, macOS, and Android.

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

Platform exposes page-based virtual-memory primitives. Memory policies, such as the arena allocator, consume these primitives from the platform API.

```cpp
U64 page_size = platform_virtual_memory_get_page_size();
Memory_Block block = platform_virtual_memory_reserve(page_size);

platform_virtual_memory_commit(block);
platform_virtual_memory_decommit(block);
platform_virtual_memory_release(block);
```

---

## Callstacks

Platform exposes callstack capture and resolve primitives. It does not log callstacks directly; callers decide how to format or report resolved frames.

```cpp
void *callstack[32] = {};
U32 frame_count = platform_callstack_capture(callstack, count_of(callstack));

Platform_Callstack_Frame frames[32] = {};
platform_callstack_resolve(callstack, frames, frame_count);
```

`Platform_Callstack_Frame` always carries the captured address. Symbol, file, and line fields are filled when the current platform and symbol data can resolve them.

---

## Path Utilities

```cpp
bool valid = platform_path_is_valid("assets/textures");
bool is_file = platform_path_is_file("assets/albedo.png");
bool is_dir  = platform_path_is_directory("assets/");

String executable = platform_path_get_executable_path();
String module = platform_path_get_current_module_path();
String cwd = platform_path_get_current_working_directory();
```

All functions accept both `String` and `const char *`.

---

## Resources

Resources are separate from normal filesystem APIs. On Windows, Linux, and macOS, resource reads use the filesystem path directly. On Android, resource reads use APK assets through Core's internal NativeActivity context.

```cpp
String shader = platform_resource_read("shaders/basic.vert", memory::temp_allocator());
Array<String> textures = platform_resource_list_files("textures", "png", memory::temp_allocator());
```

`platform_file_read` and `platform_path_read_file` do not fall back to APK assets.

---

## Environment

```cpp
String path = platform_environment_variable_get("PATH");
```

Missing environment variables return an empty `String`.

---

## Android

Android support is NDK-only. Core uses Android system APIs and does not depend on GameActivity, AndroidX, Jetpack, Gradle libraries, or `android_native_app_glue`.

The app owns the raw `ANativeActivity_onCreate` entrypoint. The Android smoke example keeps a tiny `core_android_native_activity` object library beside the example that hands `ANativeActivity` to Core and starts the example main function. A real app repo should use the same ownership shape in its own app layer.

```cpp
#include <core/platform/platform.h>

void
android_app_loop()
{
	Platform_Window window = platform_window_init(1280, 720, "Core Android App");

	while (platform_window_poll(&window))
	{
		void *native_window = nullptr;
		void *native_activity = nullptr;
		platform_window_get_native_handles(&window, &native_window, &native_activity);

		if (!native_window)
			continue;
	}

	platform_window_deinit(&window);
}
```

`platform_window_get_native_handles` returns `ANativeWindow *` as `native_handle` and `ANativeActivity *` as `native_connection` on Android.
Android windows use the normal `platform_window_init` entry point. The app-side NativeActivity shim initializes Core before app code creates the window.

Soft keyboard text input is not part of the initial Android backend. If needed, it should be added as a small Core-owned IME bridge instead of adopting GameActivity.

---

## Dialogs

```cpp
char path[4096] = {};
bool selected = platform_file_dialog_open(path, count_of(path), "Scene (*.scene)\0*.scene\0");

if (selected)
    load_scene(path);
```

```cpp
char path[4096] = {};
bool selected = platform_file_dialog_save(path, count_of(path), "Scene (*.scene)\0*.scene\0");
```

File dialogs are currently implemented on Windows and macOS. Linux and Android return `false` because Core does not depend on external dialog providers such as toolkit helpers, desktop portals, or Android activity intents.

---

## Platform Macros

The build system defines these so you can conditionally compile:

| Macro | Platform |
|---|---|
| `PLATFORM_WINDOWS` | Windows |
| `PLATFORM_LINUX` | Linux |
| `PLATFORM_MACOS` | macOS |
| `PLATFORM_ANDROID` | Android |
| `COMPILER_MSVC` | MSVC |
| `COMPILER_CLANG` | Clang / Apple Clang |
| `COMPILER_GCC` | GCC |
| `DEBUG` | Debug / RelWithDebInfo builds |
| `RELEASE` | Release / MinSizeRel builds |