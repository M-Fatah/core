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

Core is a library, not an APK packager. The app repo owns the raw `ANativeActivity_onCreate` entrypoint, app thread, manifest, package name, permissions, assets, signing, install, and launch steps. For Android framework features that cannot be reached from pure NDK APIs, Core generates a tiny `core.android.CoreNativeActivity` Java bridge in the build directory. The app repo compiles and packages that generated Java source with the APK.

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

The Android app thread must leave its loop when `platform_window_poll` returns false and must call `platform_window_deinit` before the process starts another Core Android window. Core requests that shutdown from Activity destroy and Back/close events.

`saved_state` is accepted by `platform_android_native_activity_on_create` because Android passes it to `ANativeActivity_onCreate`, but Core does not interpret or restore those bytes. The app owns durable state restoration through its own save files, project files, or app-specific serialization. Core recreates platform objects such as the Activity context, native window, input queue, asset manager, and Java bridge; app/game/editor state must be rebuilt by the client.

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

get_target_property(CORE_ANDROID_JAVA_SOURCE_DIR core CORE_ANDROID_JAVA_SOURCE_DIR)
```

The app manifest declares Core's generated `NativeActivity` subclass and names the shared library without the `lib` prefix:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.myapp">

    <uses-sdk android:minSdkVersion="26" />

    <application
        android:extractNativeLibs="true"
        android:hasCode="true"
        android:label="My App">
        <provider
            android:name="core.android.CoreClipboardProvider"
            android:authorities="com.example.myapp.core.clipboard"
            android:exported="true"
            android:grantUriPermissions="true" />
        <activity
            android:name="core.android.CoreNativeActivity"
            android:configChanges="colorMode|density|fontScale|keyboard|keyboardHidden|layoutDirection|locale|mcc|mnc|navigation|orientation|screenLayout|screenSize|smallestScreenSize|touchscreen|uiMode"
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

The `configChanges` list is part of the Core NativeActivity contract. Android should deliver those changes to the existing Activity while Core owns the app loop; if an app intentionally allows Activity recreation, it must wait until the old app thread exits and `platform_window_deinit` has released the Core Android context before initializing Core again.

APK packaging belongs in the app repo. A CMake-only app can package with Android SDK tools (`aapt2`, `zipalign`, `apksigner`, `adb`), or the app can use Gradle. Either way, package `libmy_android_app.so` into `lib/<abi>/`, package Core resources into APK `assets/`, sign the APK, install it, then launch the manifest package.

When packaging without Gradle, compile the generated Java source directory reported by `CORE_ANDROID_JAVA_SOURCE_DIR` into `classes.dex` and package it into the APK. When using Gradle, add that directory to the app's Java source set. The clipboard provider authority must be `<runtime package name>.core.clipboard`; with Gradle, `android:authorities="${applicationId}.core.clipboard"` is usually the right manifest form.

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

Android soft keyboard input uses Core's generated `CoreNativeActivity` bridge. The bridge creates the Android `InputConnection` endpoint internally; app code only enables a text-input session through Core and consumes text-input events from `platform_window_poll`.

---

## Text Input

Text input is an OS input-method transport, not a Core text editor. The app owns its text buffer, cursor, selection, validation, undo, and submit behavior. Core only asks the OS to route text input to a window and reports the commands produced by the OS input method.

```cpp
Platform_Text_Input_Desc text_input {
	.x = caret_x,
	.y = caret_y,
	.width = caret_width,
	.height = caret_height,
	.enabled = true
};
platform_window_text_input_set(window, text_input);

while (platform_window_poll(&window))
{
	for (U64 i = 0; i < window.input.text_input_events.count; ++i)
	{
		const Platform_Text_Input_Event &event = window.input.text_input_events[i];
		// Apply commit, compose, delete-surrounding, or action to the app-owned text buffer.
	}
}

text_input.enabled = false;
platform_window_text_input_set(window, text_input);
```

`Platform_Text_Input_Event::text` is UTF-8, platform-owned, and valid until the next `platform_window_poll` for that window; copy it if it must outlive the frame. `COMMIT` is finalized text, `COMPOSE` is transient preedit/composition text, `COMPOSE_END` clears composition, `DELETE_SURROUNDING` requests deletion around the app-owned cursor, and `ACTION` reports OS editor actions such as Done, Search, or Send. Coordinates in `Platform_Text_Input_Desc` use Core window coordinates.

Android uses the generated Java `InputConnection` bridge, macOS uses `NSTextInputClient`, Windows uses `WM_CHAR`, and Linux/X11 currently reports basic key-symbol text; full Linux IME composition should be added through XIM/IBus/Fcitx integration when needed.

---

## Dialogs

```cpp
String path = platform_file_dialog_open("Scene (*.scene)\0*.scene\0", memory::temp_allocator());

if (path.count > 0)
    load_scene(path.data);
```

```cpp
String path = platform_file_dialog_save("Scene (*.scene)\0*.scene\0", memory::temp_allocator());
```

File dialogs are currently implemented on Windows, Linux, macOS, and Android. Linux uses `xdg-desktop-portal` through D-Bus because it is toolkit-neutral and works across modern X11, Wayland, and sandboxed desktops; helper executables such as `zenity` are intentionally avoided.

Android dialogs use the system Storage Access Framework through Core's generated `CoreNativeActivity` bridge. The selected value is a `content://` URI, not a normal filesystem path. Pass that URI back into Core file APIs such as `platform_file_read`, `platform_file_write`, `platform_file_open`, and `platform_path_read_file`; do not pass it to non-Core POSIX APIs. Android extension filters are best effort because SAF filters by MIME type.

```cpp
String directory = platform_directory_dialog_open(memory::temp_allocator());
Array<String> files = platform_path_list_files(directory, "png", memory::temp_allocator());
```

On Android, `platform_directory_dialog_open` returns a tree `content://` URI with persistable read/write prefix permission when the selected provider grants it. `platform_path_list_files` returns child document URIs for tree content URIs; pass those child URIs back into Core file APIs. `platform_file_delete` uses Android document deletion for content URIs. Document rename is not exposed yet because Android returns a new URI from rename and Core does not currently have a rename API that can return it.

---

## Clipboard

```cpp
Array<String> media_types = platform_window_clipboard_query_media_types(window, memory::temp_allocator());

Platform_Clipboard_Item item = platform_window_clipboard_item_read(window, string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8), memory::temp_allocator());
if (item.data.count > 0)
{
	// item.data contains UTF-8 bytes.
}
```

```cpp
String text = string_literal("Copied from Core");
Platform_Clipboard_Item item {
	.media_type = string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8),
	.data = array_init_from((const U8 *)text.data, (const U8 *)text.data + text.count, memory::temp_allocator())
};

platform_window_clipboard_item_write(window, &item, 1);
```

Clipboard APIs take a `Platform_Window &` because Linux/X11 clipboard ownership is window-based: Core owns the `CLIPBOARD` selection through that window, keeps copied items alive, answers `SelectionRequest` events from `platform_window_poll`, and clears ownership on `SelectionClear`. Wayland should be handled separately through its data-device protocol once Core has a Wayland backend.

Clipboard items use standard media type strings such as `text/plain;charset=utf-8`, `image/png`, and `application/octet-stream`. Core transports bytes and reports the media type; the caller owns interpretation, encoding, decoding, and validation. Windows, Linux/X11, macOS, and Android support exact media-type byte items. On Android, non-text clipboard writes are backed by Core's generated `CoreClipboardProvider`, so the app manifest must include the provider entry shown above.

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