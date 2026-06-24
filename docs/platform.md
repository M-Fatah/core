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

Core is a library, not an APK packager. The app repo owns the raw `ANativeActivity_onCreate` entrypoint, app thread, manifest, package name, permissions, assets, signing, install, and launch steps.

The app entrypoint hands `ANativeActivity` to Core before app code creates a platform window:

```cpp
#include <core/defines.h>
#include <core/validate.h>

#include <android/native_activity.h>
#include <pthread.h>

extern "C" void
platform_android_native_activity_on_create(void *native_activity, void *saved_state, U64 saved_state_size);

extern "C" void
app_main();

inline static void *
_android_app_thread_main(void *)
{
	app_main();
	return nullptr;
}

extern "C" __attribute__((visibility("default"))) void
ANativeActivity_onCreate(ANativeActivity *activity, void *saved_state, size_t saved_state_size)
{
	platform_android_native_activity_on_create(activity, saved_state, (U64)saved_state_size);

	pthread_t thread = {};
	validate(::pthread_create(&thread, nullptr, _android_app_thread_main, nullptr) == 0, "[ANDROID]: Failed to create app thread.");
	validate(::pthread_detach(thread) == 0, "[ANDROID]: Failed to detach app thread.");
}
```

The app shared library links Core:

```cmake
set(CORE_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(CORE_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(CORE_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(CORE_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(path/to/core core)

add_library(my_android_app SHARED
    src/android_native_activity_entry.cpp
    src/app.cpp
)
target_link_libraries(my_android_app PRIVATE core)
```

The app manifest declares `NativeActivity` and names the shared library without the `lib` prefix:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.myapp">

    <uses-sdk android:minSdkVersion="26" />

    <application
        android:extractNativeLibs="true"
        android:hasCode="false"
        android:label="My App">
        <activity
            android:name="android.app.NativeActivity"
            android:configChanges="keyboardHidden|orientation|screenSize"
            android:exported="true"
            android:label="My App">
            <meta-data
                android:name="android.app.lib_name"
                android:value="my_android_app" />
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>
```

APK packaging belongs in the app repo. A CMake-only app can package with Android SDK tools (`aapt2`, `zipalign`, `apksigner`, `adb`), or the app can use Gradle. Either way, package `libmy_android_app.so` into `lib/<abi>/`, package Core resources into APK `assets/`, sign the APK, install it, then launch the manifest package.

```cpp
#include <core/platform/platform.h>

void
android_app_loop()
{
	Platform_Window window = platform_window_init(1280, 720, "Core Android App");

	while (platform_window_poll(&window))
	{
		if (window.paused || !window.surface_valid)
		{
			platform_sleep(16);
			continue;
		}

		Platform_Window_Native_Handles native = platform_window_get_native_handles(&window);

		if (!native.window)
			continue;

		if (window.surface_changed)
		{
			// Recreate renderer surface/swapchain here.
		}
	}

	platform_window_deinit(&window);
}
```

`platform_window_get_native_handles` returns `ANativeWindow *` as `window` and `ANativeActivity *` as `context` on Android.
Returned handles are borrowed. On Android, Core keeps the returned `ANativeWindow *` valid until the next `platform_window_poll` or `platform_window_deinit`; do not cache it across frames.
`Platform_Window::surface_valid` reports whether a native render surface currently exists. `surface_changed` is true on the first poll after window creation and when the native surface or size changes, then is refreshed by the next poll.
`Platform_Window::paused` is driven by Android Activity pause/resume and remains false on desktop platforms.
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