# Platform

**Header:** `core/platform/platform.h`

Cross-platform file I/O, path utilities, native dialogs, windows, and low-level platform primitives. Implementations are provided for Windows, Linux, macOS, Android, and iOS.

---

## Platform Support

| Platform | Status | Backend |
|---|---|---|
| Windows | Supported | `platform_win32.cpp` |
| Linux | Supported | `platform_linux.cpp` |
| macOS | Supported | `platform_macos.mm` |
| Android | Supported | `platform_android.cpp` |
| iOS | Validation pending | UIKit, document providers, and consumer-owned Metal surfaces |

A platform is supported only when its backend builds, links, and passes its platform validation. The iOS backend covers ordinary non-UI platform services, except runtime API hot reload, which iOS does not support. It also provides explicit per-scene UIKit window ownership, lifecycle and input integration, clipboard transport, document-token file and path operations, system document pickers, raw-byte save export, and borrowed native handles for consumer-owned Metal rendering. Simulator CI and physical-device validation must pass before iOS is marked supported.

---

## File I/O

```cpp
#include <core/platform/platform.h>

// Read entire file into a String
String contents = platform_path_read_file("data/config.json");
DEFER(string_deinit(contents));

// With explicit allocator
String shader = platform_path_read_file("shader.glsl", memory::temp_allocator());

// Read into caller-owned memory through a file handle
Memory_Block block = memory::allocate(file_size, alignof(U8));
DEFER(memory::deallocate(block));

Platform_File_Handle file = platform_file_open("data.bin", PLATFORM_FILE_MODE_READ);
U64 bytes_read = 0;
if (file != PLATFORM_FILE_HANDLE_INVALID)
{
    DEFER(platform_file_close(file));
    bytes_read = platform_file_read(file, block.data, block.size);
}
U64 bytes_written = platform_path_write_file("copy.bin", block);
```

`platform_path_read_file` owns its returned `String` through the requested allocator and preserves binary bytes, including embedded nulls. `platform_path_write_file` creates or truncates its destination and returns the number of bytes written. Handle-based reads and writes operate on caller-owned memory.

On iOS, normal file I/O is subject to the application sandbox. Use paths under the app-data, cache, or temporary directories for writable files. Bundled resources remain separate and must be read through `platform_resource_read`.

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

iOS virtual-memory reservations are committed as read/write, non-executable memory and do not require JIT entitlements.

---

## Threads

```cpp
Platform_Thread *thread = platform_thread_init(Platform_Thread_Desc {
	.function = audio_thread_entry,
	.data = audio_state,
	.name = "Audio"
});

platform_thread_join(thread);
platform_thread_deinit(thread);

platform_thread_set_current_name("Main");
platform_thread_sleep(16);
```

`Platform_Thread_Desc::name` is optional. Core copies the name during `platform_thread_init`, so the descriptor string does not need to outlive the call. Thread names are for debuggers, profilers, and platform tools; they do not affect scheduling.

Platform thread-name APIs have native length limits, especially on POSIX platforms. Passing a name rejected by the OS fails validation.

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
U64 file_size = platform_path_get_file_size("assets/albedo.png");

String executable = platform_path_get_executable_path();
String module = platform_path_get_current_module_path();
String cwd = platform_path_get_current_working_directory();
String temp = platform_path_get_temp_directory();
String app_data = platform_path_get_app_data_directory();
String cache = platform_path_get_cache_directory();
```

All functions accept both `String` and `const char *`.

`platform_path_get_app_data_directory` returns the platform's persistent app-data base directory with a trailing slash. `platform_path_get_cache_directory` returns the platform's cache-data base directory with a trailing slash. On desktop platforms these are user-level base directories, so apps should create their own product subdirectory inside them. On Android they are the package-private files and cache directories. On iOS they are the app's sandboxed `Library/Application Support` and `Library/Caches` directories; the temporary-directory API returns the sandboxed `tmp` directory.

Path mutation APIs return the resulting path. Provider-backed results remain opaque: Android can return a new `content://` URI, while iOS returns a new `core-document://` token after create, rename, or move.

On iOS, ordinary path APIs operate on filesystem paths accessible through the application sandbox and on opaque document tokens granted by a system picker. Token predicates, size and name queries, file copy/deletion, creation, rename, move, and directory listing are coordinated through the document provider. A token directory is its own directory result; a file token returns a parent token only when the provider grant also authorizes that parent. Token directory listings return child tokens that can be passed directly to every supported Core file/path API. Process working-directory APIs accept filesystem paths only. Bundle resources remain separate; use `platform_resource_read` and `platform_resource_list_files` for them.

```cpp
String file = platform_path_create_file(directory, "scene.scene", memory::temp_allocator());
String folder = platform_path_create_directory(directory, "Textures", memory::temp_allocator());
String renamed = platform_path_rename(file, "level.scene", memory::temp_allocator());
String moved = platform_path_move(renamed, folder, memory::temp_allocator());
bool copied = platform_path_copy_file(moved, "level_backup.scene");
bool deleted = platform_path_delete_file("level_backup.scene");

Array<String> pngs = platform_path_list_files_recursive(folder, "png", memory::temp_allocator());
```

`platform_path_create_file` and `platform_path_create_directory` create a child inside the supplied directory. `platform_path_rename` keeps the item in its current directory and changes only its name. `platform_path_move` moves the item into the supplied destination directory and keeps the current name. File copy replaces an existing destination; file deletion does not delete directories.

---

## Resources

Resources are separate from normal filesystem APIs. On Windows, Linux, and macOS, resource reads use the filesystem path directly. On Android, resource reads use APK assets through Core's internal NativeActivity context. On iOS, resource paths are relative to the read-only resource directory of the main application bundle.

```cpp
String shader = platform_resource_read("shaders/basic.vert", memory::temp_allocator());
Array<String> textures = platform_resource_list_files("textures", "png", memory::temp_allocator());
```

`platform_resource_list_files` returns file names relative to the requested directory and does not include subdirectories. iOS resource paths must be relative and cannot escape the application bundle. `platform_path_read_file` does not fall back to Android APK assets or iOS bundle resources.

---

## Environment

```cpp
String path = platform_environment_variable_get("PATH");
```

Missing environment variables return an empty `String`.

---

## Hot Reload

`platform_api_init`, `platform_api_load`, and `platform_api_deinit` provide runtime native-code hot reload on platforms that permit it. iOS does not permit Core's copy-and-load dylib workflow because application executable code must remain signed and self-contained. On iOS, initialization and loading fail validation in debug builds and return an empty API or `nullptr` in release builds; deinitialization remains safe.

---

## iOS

The root build provides an iOS-only `unittest_ios_host` application bundle and a `unittest` XCTest bundle. The host owns the `UIApplicationMain` entrypoint, configures `UIWindowScene` instances, packages a known resource into its main bundle, and verifies scene lifecycle state through the existing Core tester. The CI workflow runs the XCTest scheme against an available iPhone simulator in Debug and Release.

```bash
cmake -S . -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DCORE_BUILD_STATIC=ON \
  -DCORE_BUILD_UNITTEST=ON

cmake --build build-ios --target unittest --config Debug
```

iOS requires iOS 13.0 or newer for the scene lifecycle and `CORE_BUILD_STATIC=ON`; the Core target validates explicitly configured deployment targets and the static-library requirement beside library creation. Select the generated `unittest_ios_host` scheme and an iOS Simulator destination in Xcode to run the bundle. A device build uses `-DCMAKE_OSX_SYSROOT=iphoneos` and requires the host application and test bundle to be signed through the consuming Xcode project.

Each `UIWindowSceneDelegate` owns one `Platform_Window`. The delegate creates the Core window with the generic `platform_window_init`, creates an empty `UIWindow` for its scene, then binds both objects with `platform_window_native_connect` from `core/platform/platform.h`. Core installs and manages the root view controller and view so generic presentation and later input policy do not depend on host-specific overrides. A fresh iOS scene returns `PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_CONNECTED`.

Core registers scene-specific UIKit lifecycle observers during that handoff and reports active, inactive, foreground, background, and disconnect transitions through `platform_window_poll`. UIKit retains ownership of the scene and window. The window retains Core's root view controller and view until Core removes them during scene disconnect or `platform_window_deinit`.

On iOS, connection, presentation updates, polling, and deinitialization run on the main thread because UIKit window state is main-thread state. Window dimensions, content rects, and safe-area insets use UIKit points. `metrics.density_scale` converts those values to device pixels, DPI uses the mobile logical 160-DPI baseline, and `surface_changed` reports changes to bounds, safe area, density, orientation, or surface validity.

`platform_window_get_native_handles` returns the connected `UIWindow *` as `window` and its root `UIView *` as `context`. Both are borrowed UIKit objects and must not be cached across scene disconnect, window deinitialization, or frames.

The consumer owns its rendering objects. A Metal consumer can attach its own layer to the borrowed view without adding rendering ownership to Core:

```objective-c++
Platform_Window_Native_Handles native = platform_window_get_native_handles(&window);
UIView *view = (UIView *)native.context;
CGRect bounds = [view bounds];
CGFloat scale = [[[view window] screen] scale];

CAMetalLayer *metal_layer = [[CAMetalLayer alloc] init];
[metal_layer setDevice:MTLCreateSystemDefaultDevice()];
[metal_layer setPixelFormat:MTLPixelFormatBGRA8Unorm];
[metal_layer setContentsScale:scale];
[metal_layer setFrame:bounds];
[metal_layer setDrawableSize:CGSizeMake(bounds.size.width * scale, bounds.size.height * scale)];
[[view layer] addSublayer:metal_layer];
```

Re-query the native handles after every `platform_window_poll`, update the owned layer's frame and drawable size when `surface_changed` is set, and stop requesting drawables while the application is paused or `surface_valid` is false. Remove and release the layer before the Core window disconnects or is deinitialized.

`platform_window_presentation_set` maps fullscreen to status-bar hiding, immersive mode to requested status-bar and Home-indicator hiding plus deferred system-edge gestures, and edge-to-edge mode to extended root-view layout. Keep-screen-on coordinates UIKit's application-wide idle timer across connected Core windows and restores its prior value after the final request is removed. Orientation policy is constrained by the app's `UISupportedInterfaceOrientations`; iOS 16 or newer receives an explicit scene-geometry request, while iOS 13 through 15 use UIKit's supported-orientation rotation path.

Core's root view enables UIKit multi-touch and reports up to `PLATFORM_TOUCH_MAX_COUNT` simultaneous contacts through `Platform_Window::input.touches`. Contact IDs remain stable until release, coordinates use Core's bottom-left origin in UIKit points, and `pressed`, `released`, `dx`, and `dy` describe all changes accumulated before the current `platform_window_poll`. A touch that begins and ends between polls reports both transitions in the same poll.

On iOS 13.4 or newer, the root view uses UIKit hover and scroll gesture recognizers for mouse and trackpad movement. Indirect pointer clicks update Core's left, middle, and right mouse-key states, while vertical scroll events accumulate in `Platform_Window::input.mouse_wheel`. Add `<key>UIApplicationSupportsIndirectInputEvents</key><true/>` to the app's `Info.plist` so iOS 13.4 through 16 report pointer clicks as `UITouchTypeIndirectPointer`; iOS 17 and newer default to this mode, but an explicit key keeps host behavior consistent across Core's supported deployment range.

On iOS 13.4 or newer, Core's root view also observes raw physical-keyboard press, release, and cancellation events. `Platform_Window::input.keys` uses UIKit HID usage codes for physical, location-oriented key state; repeated down callbacks keep a key down without adding another press transition. Keyboard-layout text and input-method composition are intentionally separate from this physical-key path.

iOS text entry uses a dedicated `UITextInput` responder owned by Core's root view. It converts the app-owned UTF-8 snapshot and byte ranges to UIKit's UTF-16 positions, exposes surrounding text to the active input method, reports marked text as composition events, and maps commit, deletion, selection, and Return-key actions back to `Platform_Text_Input_Event`. Disabling text input dismisses the software keyboard and restores the root view as the physical-key responder.

iOS document references use the versioned, self-contained `core-document://v1/<bookmark>` token format. The payload is canonical unpadded Base64URL bookmark data, so tokens require no process-global URL registry and can be persisted as ordinary strings. Tokens remain opaque to applications. File I/O and path operations resolve these tokens and coordinate access through the document provider. Whole-file and path operations release security access before returning; an open handle retains access until `platform_file_close`. The open-file and directory pickers produce the same token format. The save picker returns only completion status because it owns the complete export operation.

Before treating an iOS release as supported, validate on a provisioned physical device: portrait and landscape safe areas, background/foreground restoration, touch and connected hardware input, multi-stage Unicode composition, clipboard privacy prompts, iCloud document access, stale bookmarks, and unavailable or offline document providers. Provider cancellation and failure must leave caller-owned data unchanged and release every security-scoped access.

The binding remains explicit and per scene. Core does not keep a global current-scene registry, `Platform_Window_Desc` does not contain UIKit context, and the scene delegate owns generic `platform_window_deinit` cleanup.

---

## Android

Android support is NDK-only. Core uses Android system APIs and does not depend on GameActivity, AndroidX, Jetpack, Gradle libraries, or `android_native_app_glue`.

Core is a library, not an APK packager. The app repo owns the raw `ANativeActivity_onCreate` entrypoint, app thread, manifest, package name, permissions, assets, signing, install, and launch steps. For Android framework features that cannot be reached from pure NDK APIs, Core generates a tiny `core.android.CoreNativeActivity` Java bridge in the build directory. The app repo compiles and packages that generated Java source with the APK.

Android uses the same host order as iOS: create a persistent `Platform_Window`, bind the native host, then start the app loop only for a fresh connection. The host mutex prevents Activity reconnection from racing window teardown:

```cpp
#include <core/platform/platform.h>
#include <core/validate.h>

#include <android/native_activity.h>
#include <pthread.h>

extern "C" void
app_main(Platform_Window *window);

inline static pthread_mutex_t _android_host_mutex = PTHREAD_MUTEX_INITIALIZER;
inline static Platform_Window _android_window = {};

inline static void *
_android_app_thread_main(void *data)
{
	Platform_Window *window = (Platform_Window *)data;
	app_main(window);

	validate(::pthread_mutex_lock(&_android_host_mutex) == 0, "[ANDROID]: Failed to lock host mutex.");
	platform_window_deinit(window);
	validate(::pthread_mutex_unlock(&_android_host_mutex) == 0, "[ANDROID]: Failed to unlock host mutex.");
	return nullptr;
}

extern "C" __attribute__((visibility("default"))) void
ANativeActivity_onCreate(ANativeActivity *activity, void *saved_state, size_t saved_state_size)
{
	(void)saved_state;
	(void)saved_state_size;

	validate(::pthread_mutex_lock(&_android_host_mutex) == 0, "[ANDROID]: Failed to lock host mutex.");
	bool initialized = _android_window.ctx == nullptr;
	if (initialized)
		_android_window = platform_window_init(Platform_Window_Desc {});

	if (_android_window.ctx == nullptr)
	{
		validate(::pthread_mutex_unlock(&_android_host_mutex) == 0, "[ANDROID]: Failed to unlock host mutex.");
		return;
	}

	PLATFORM_WINDOW_NATIVE_CONNECT_RESULT connect_result =
		platform_window_native_connect(&_android_window, activity, nullptr);
	validate(connect_result != PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED, "[ANDROID]: Failed to connect NativeActivity.");
	if (connect_result == PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_FAILED)
	{
		if (initialized)
			platform_window_deinit(&_android_window);
		validate(::pthread_mutex_unlock(&_android_host_mutex) == 0, "[ANDROID]: Failed to unlock host mutex.");
		return;
	}

	if (connect_result == PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_RECONNECTED)
	{
		validate(::pthread_mutex_unlock(&_android_host_mutex) == 0, "[ANDROID]: Failed to unlock host mutex.");
		return;
	}

	pthread_t thread = {};
	I32 thread_result = ::pthread_create(&thread, nullptr, _android_app_thread_main, &_android_window);
	validate(thread_result == 0, "[ANDROID]: Failed to create app thread.");
	if (thread_result != 0)
		platform_window_deinit(&_android_window);
	else
		validate(::pthread_detach(thread) == 0, "[ANDROID]: Failed to detach app thread.");
	validate(::pthread_mutex_unlock(&_android_host_mutex) == 0, "[ANDROID]: Failed to unlock host mutex.");
}
```

The host owns the persistent window and deinitializes it after `app_main` returns. The app loop must return when `platform_window_poll` returns false. `PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_RECONNECTED` means Android rebound a recreated Activity to the existing window and app thread, so the entrypoint must not start another thread.

Android passes saved-state bytes to `ANativeActivity_onCreate`, but Core does not consume them and the host does not forward them. The app owns durable state restoration through its own save files, project files, or app-specific serialization. `Platform_Window::save_state_requested` is a one-poll signal that Android requested state persistence; use it to flush app-owned state. Core recreates platform objects such as the Activity context, native window, input queue, asset manager, and Java bridge; app/game/editor state must be rebuilt by the client.

The app shared library links Core:

```cmake
set(CORE_BUILD_STATIC ON CACHE BOOL "" FORCE)
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

`CoreNativeActivity` loads the app native library named by `android.app.lib_name` before its lifecycle callbacks call Core native bridge methods. Keep that metadata on the activity even if the app also has its own package, signing, and APK build flow.

The `configChanges` list is still recommended because it avoids avoidable Activity churn while Core owns the app loop. If Android recreates `core.android.CoreNativeActivity` for a configuration change, Core preserves the existing window context, rebinds the new Activity, reapplies window presentation policy, and returns `PLATFORM_WINDOW_NATIVE_CONNECT_RESULT_RECONNECTED`. If the Activity is finishing, the existing app loop must return through `platform_window_poll`; the host then deinitializes the window before another connection can start a new thread.

APK packaging belongs in the app repo. A CMake-only app can package with Android SDK tools (`aapt2`, `zipalign`, `apksigner`, `adb`), or the app can use Gradle. Either way, package `libmy_android_app.so` into `lib/<abi>/`, package Core resources into APK `assets/`, sign the APK, install it, then launch the manifest package.

When packaging without Gradle, compile the generated Java source directory reported by `CORE_ANDROID_JAVA_SOURCE_DIR` into `classes.dex` and package it into the APK. When using Gradle, add that directory to the app's Java source set. The clipboard provider authority must be `<runtime package name>.core.clipboard`; with Gradle, `android:authorities="${applicationId}.core.clipboard"` is usually the right manifest form.

```cpp
#include <core/platform/platform.h>

extern "C" void
app_main(Platform_Window *window)
{
	while (platform_window_poll(window))
	{
		if (window->paused || !window->surface_valid)
		{
			platform_thread_sleep(16);
			continue;
		}

		Platform_Window_Native_Handles native = platform_window_get_native_handles(window);

		if (!native.window)
			continue;

		if (window->surface_changed)
		{
			// Recreate renderer surface/swapchain here.
		}
	}
}
```

`Platform_Window_Desc` supplies the initial desktop window size and title. Android keeps the same construction API, but its actual size comes from the current `ANativeWindow` and its displayed app title comes from the Activity manifest; Core does not override either value from the descriptor.

`platform_window_get_native_handles` returns `ANativeWindow *` as `window` and `ANativeActivity *` as `context` on Android.
Returned handles are borrowed. On Android, Core keeps the returned `ANativeWindow *` valid until the next `platform_window_poll` or `platform_window_deinit`; do not cache it across frames.
`Platform_Window::surface_valid` reports whether a native render surface currently exists. `surface_changed` is true on the first poll after window creation and when the native surface, size, content rect, or display metrics change, then is refreshed by the next poll.
`Platform_Window::metrics` reports the current content rect, safe-area insets, density scale, DPI, and portrait/landscape orientation. Rects and insets are in Core window coordinates: origin at the bottom-left, with the same units as `Platform_Window::width` and `Platform_Window::height`. On Android, safe area and content rect come from NativeActivity's content-rect callback. On iOS, they come from the connected root view's bounds and safe-area insets. Desktop content rects cover the window client/content area unless the OS reports safe insets.
`platform_window_presentation_set` requests fullscreen, immersive, keep-screen-on, edge-to-edge, and orientation policy through a generic Core API:

```cpp
Platform_Window_Presentation_Desc presentation {
	.flags = PLATFORM_WINDOW_PRESENTATION_FLAG_FULLSCREEN |
	         PLATFORM_WINDOW_PRESENTATION_FLAG_IMMERSIVE |
	         PLATFORM_WINDOW_PRESENTATION_FLAG_KEEP_SCREEN_ON |
	         PLATFORM_WINDOW_PRESENTATION_FLAG_EDGE_TO_EDGE,
	.orientation_policy = PLATFORM_WINDOW_ORIENTATION_POLICY_SENSOR_LANDSCAPE
};

platform_window_presentation_set(window, presentation);
```

`Platform_Window::presentation` stores the requested state. `Platform_Window::metrics.safe_area` stores the observed area that should stay unobstructed by system UI. On Android, fullscreen, immersive, keep-screen-on, edge-to-edge/cutout behavior, and orientation policy are applied through Core's generated `CoreNativeActivity` bridge. On iOS, Core's root view controller applies the corresponding UIKit presentation preferences and scene orientation request. On desktop, fullscreen maps to the native fullscreen path, keep-screen-on maps to the platform display-inhibit API, and orientation policy is stored but has no OS window meaning.
`Platform_Window::started` tracks Android Activity start/stop visibility and remains true on desktop platforms.
`Platform_Window::paused` is driven by Android Activity pause/resume and remains false on desktop platforms.
`Platform_Window::low_memory` is a one-poll platform memory-pressure signal. On Android it is driven by `onLowMemory` and memory-pressure `onTrimMemory` levels; on desktop it remains false.
`Platform_Window::save_state_requested` is a one-poll platform request to persist app-owned state. On Android it is driven by `onSaveInstanceState`; Core returns no Android saved-state blob.
Android windows use the normal `platform_window_init` entry point before `platform_window_native_connect`, matching iOS host setup.

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
	.flags = PLATFORM_TEXT_INPUT_FLAG_MULTILINE,
	.action = PLATFORM_TEXT_INPUT_ACTION_DONE,
	.text = editor_text,
	.selection_start = cursor_byte_offset,
	.selection_end = cursor_byte_offset,
	.enabled = true
};
platform_window_text_input_set(window, text_input);

while (platform_window_poll(&window))
{
	for (U64 i = 0; i < window.input.text_input_events.count; ++i)
	{
		const Platform_Text_Input_Event &event = window.input.text_input_events[i];
		// Apply commit, compose, selection, delete-surrounding, or action to the app-owned text buffer.
	}

	text_input.text = editor_text;
	text_input.selection_start = cursor_byte_offset;
	text_input.selection_end = cursor_byte_offset;
	platform_window_text_input_set(window, text_input);
}

text_input.enabled = false;
platform_window_text_input_set(window, text_input);
```

`Platform_Text_Input_Desc::text` is an app-owned UTF-8 snapshot. Core consumes it during `platform_window_text_input_set`; callers do not transfer ownership. Selection and composition ranges are UTF-8 byte offsets into that snapshot. Keep the snapshot current when the app text buffer or cursor changes so IMEs can query surrounding text correctly.

`Platform_Text_Input_Event::text` is UTF-8, platform-owned, and valid until the next `platform_window_poll` for that window; copy it if it must outlive the frame. `COMMIT` is finalized text, `COMPOSE` is transient preedit/composition text, `COMPOSE_END` clears composition, `COMPOSE_REGION` marks an existing byte range as composing text, `SELECTION` moves the app-owned selection, `DELETE_SURROUNDING` requests byte deletion around the app-owned selection, and `ACTION` reports OS editor actions such as Done, Search, or Send. Coordinates in `Platform_Text_Input_Desc` use Core window coordinates.

Text input flags describe the requested OS input mode: multiline, password, number, decimal, signed, email, URI, and no-suggestions. Android maps them to `InputType`, iOS maps them to `UITextInputTraits`, macOS uses the text snapshot through `NSTextInputClient`, Windows uses `WM_CHAR`, and Linux/X11 currently reports basic key-symbol text; full Linux IME composition should be added through XIM/IBus/Fcitx integration when Core grows a Linux IME backend.

---

## Dialogs

```cpp
String path = platform_file_dialog_open(window, "Scene (*.scene)\0*.scene\0", memory::temp_allocator());

if (path.count > 0)
    load_scene(path.data);
```

```cpp
Slice<const U8> scene_data = serialize_scene(scene);
bool saved = platform_file_dialog_save(
	window,
	"level.scene",
	scene_data,
	"Scene (*.scene)\0*.scene\0");
```

File dialogs require an explicit `Platform_Window &` presenter. Windows uses its HWND as the dialog owner, Linux passes its X11 window identifier to `xdg-desktop-portal`, Android presents through the Activity associated with the window, and macOS currently runs the panel application-modally. Linux uses the portal because it is toolkit-neutral and works across modern X11, Wayland, and sandboxed desktops; helper executables such as `zenity` are intentionally avoided.

`platform_file_dialog_save` borrows the data until the synchronous call returns. The suggested name is only the editable filename initially shown by the dialog; the user chooses the final name and destination. The function returns `true` only after writing the complete data, and `false` on cancellation or failure. Empty data is valid.

On iOS, Core writes the borrowed bytes to a unique temporary sandbox file, presents that file through the system export picker as a copy, and removes the staging file and directory after success or cancellation. The staged basename is the suggested editable name. Its extension is also UIKit's export type hint; iOS does not expose a separate export-filter control. Call the synchronous save API from a non-main engine thread so the UIKit main thread remains available. Core returns `true` only after the picker reports a new copy at the selected destination.

Android dialogs use the system Storage Access Framework through Core's generated `CoreNativeActivity` bridge. Selected open and directory values are `content://` URIs, not normal filesystem paths. Pass those URIs back into Core file APIs such as `platform_path_read_file`, `platform_path_write_file`, and `platform_file_open`; do not pass them to non-Core POSIX APIs. Save destinations remain internal to `platform_file_dialog_save`. Android extension filters are best effort because SAF filters by MIME type. Some `content://` providers expose stream-like file descriptors that cannot seek; `platform_file_seek` returns false for those handles, and `platform_file_tell` returns 0 when the provider rejects tell/seek.

On iOS, `platform_file_dialog_open` presents `UIDocumentPickerViewController` from the supplied window and returns a `core-document://` bookmark token. Pass that token back into Core file and path APIs; it is intentionally not a POSIX path. Core translates filename-extension filters to document types on a best-effort basis and falls back to all data files. Because the public API is synchronous while UIKit presentation is asynchronous, call it from a non-main engine thread; Core dispatches presentation to the main thread and waits only on the calling engine thread. Cancellation, unavailable presentation, and bookmark failure return an empty string.

```cpp
String directory = platform_directory_dialog_open(window, memory::temp_allocator());
Array<String> files = platform_path_list_files(directory, "png", memory::temp_allocator());
```

On Android, `platform_directory_dialog_open` returns a tree `content://` URI with persistable read/write prefix permission when the selected provider grants it. `platform_path_list_files` and `platform_path_list_files_recursive` return child document URIs for tree content URIs; pass those child URIs back into Core file APIs. `platform_path_create_file`, `platform_path_create_directory`, `platform_path_rename`, and `platform_path_move` use Android document APIs and return the resulting URI. `platform_path_delete_file` uses Android document deletion for content URIs.

On iOS, `platform_directory_dialog_open` restricts the document picker to folders and returns a security-scoped directory token. Listing functions return bookmark tokens for children, and the document-token create, rename, move, copy, and delete operations work within the selected directory scope. The same non-main-thread calling rule and empty-result cancellation behavior as the open-file picker apply.

---

## Clipboard

```cpp
Array<String> media_types = platform_window_clipboard_query_media_types(window, memory::temp_allocator());

Array<U8> data = array_init<U8>(memory::temp_allocator());
if (platform_window_clipboard_item_read(window, string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8), data))
{
	// data contains UTF-8 bytes and may be empty.
}
```

```cpp
String text = string_literal("Copied from Core");
Platform_Clipboard_Item_Desc item {
	.media_type = string_literal(PLATFORM_CLIPBOARD_MEDIA_TYPE_TEXT_UTF8),
	.data = slice_from((const U8 *)text.data, text.count)
};

platform_window_clipboard_item_write(window, &item, 1);
```

Clipboard APIs take a `Platform_Window &` because Linux/X11 clipboard ownership is window-based: Core owns the `CLIPBOARD` selection through that window, keeps copied items alive, answers `SelectionRequest` events from `platform_window_poll`, and clears ownership on `SelectionClear`. Wayland should be handled separately through its data-device protocol once Core has a Wayland backend.

Clipboard reads use a caller-owned initialized `Array<U8>`, reuse its capacity, and clear it on failure. A successful read may contain zero bytes. Clipboard item descriptions passed to `platform_window_clipboard_item_write` are non-owning and Core copies their contents before returning. Clipboard items use standard media type strings such as `text/plain;charset=utf-8`, `image/png`, and `application/octet-stream`. Core transports bytes; the caller owns interpretation, encoding, decoding, and validation. Windows, Linux/X11, macOS, Android, and iOS support exact media-type byte items. iOS stores all descriptors from one write as representations of one system pasteboard item. Its media-type query only inspects representation types, but an item read may cause the system paste notification or permission UI when iOS cannot infer user intent. On Android, non-text clipboard writes are backed by Core's generated `CoreClipboardProvider`, so the app manifest must include the provider entry shown above.

---

## Platform Macros

The build system defines these so you can conditionally compile:

| Macro | Platform |
|---|---|
| `PLATFORM_WINDOWS` | Windows |
| `PLATFORM_LINUX` | Linux |
| `PLATFORM_MACOS` | macOS |
| `PLATFORM_IOS` | iOS |
| `PLATFORM_ANDROID` | Android |
| `COMPILER_MSVC` | MSVC |
| `COMPILER_CLANG` | Clang / Apple Clang |
| `COMPILER_GCC` | GCC |
| `DEBUG` | Debug / RelWithDebInfo builds |
| `RELEASE` | Release / MinSizeRel builds |