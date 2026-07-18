# Core

[![Build status](https://github.com/M-Fatah/core/workflows/CI/badge.svg)](https://github.com/M-Fatah/core/actions?workflow=CI)

Core is a C-like C++20 foundation library for data-oriented applications. It provides the pieces an app, tool, game engine, compiler, or renderer usually wants close at hand: memory, containers, math, formatting, logging, validation, serialization, ECS, and platform services.

The library favors explicit ownership, visible allocations, plain structs, free functions, and `_init` / `_deinit` lifetimes over hidden runtime behavior.

> Core is still a WIP. Breaking changes are expected.

## Design

- Explicit allocations: containers accept a `memory::Allocator *`.
- Explicit lifetimes: owned resources use visible init/deinit pairs.
- No exceptions: failures are returned or validated at the boundary where they matter.
- C-like C++: plain data, free functions, and simple translation units.
- Platform-first: Windows, Linux, macOS, and Android backends live inside Core without third-party runtime dependencies.

## Modules

| Module | Description |
|---|---|
| `core/defines.h` | Primitive aliases, utility macros, platform/compiler defines |
| `core/memory/` | Heap, arena, pool, temp allocator, virtual-memory-backed allocation |
| `core/containers/` | Array, string, slice, ring buffer, hash table, hash set, stack array |
| `core/math/` | Scalar helpers, vectors, matrices, quaternion, random, NEON / AVX / scalar paths |
| `core/formatter.h` | Type-safe formatting with Core strings and math types |
| `core/print.h`, `core/log.h` | Colored printing and log helpers |
| `core/defer.h` | Scope-exit cleanup macro |
| `core/validate.h` | Debug runtime validation with source locations |
| `core/tester.h` | Small unit-test framework |
| `core/result.h` | Result/error-returning pattern |
| `core/hash.h` | FNV-32 and type-generic hashing |
| `core/command_line.h` | Descriptor-driven command-line parser |
| `core/reflect.h` | Compile-time type reflection helpers |
| `core/serialization/` | Binary and JSON serializers |
| `core/ecs.h` | Minimal ECS |
| `core/scheduler.h` | Long-lived worker-thread scheduler |
| `core/platform/platform.h` | Files, paths, resources, windows, input, timing, threads, callstacks |

Full documentation lives in [`docs/`](docs/home.md).

## Quick Example

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

## Platforms

| Platform | Backend |
|---|---|
| Windows | Win32 |
| Linux | X11/XCB, xdg-desktop-portal dialogs |
| macOS | Cocoa |
| iOS | UIKit (in progress) |
| Android | NDK NativeActivity |

Android support is NDK-only: no GameActivity, AndroidX, Jetpack, Gradle dependency, or `android_native_app_glue`. Core generates tiny Java `NativeActivity` and clipboard provider classes for Android framework features such as app/cache directories, window presentation, file dialogs, document URIs, clipboard, and soft keyboard input.

iOS support is in progress. Core currently provides the non-UI backend plus a per-scene UIKit application host, native surface handles, display metrics, presentation policy, touch, mouse/trackpad, physical-keyboard, software-keyboard text input, clipboard data, self-contained document-token file and path operations, open-file and directory document pickers, and raw-byte save export, plus an XCTest bundle for validation.

## Prerequisites

| Platform | Requirements |
|---|---|
| Windows | CMake 3.29+ |
| Linux | CMake 3.29+, pkg-config, X11/XCB and D-Bus development packages |
| macOS | CMake 3.29+, Xcode Command Line Tools |
| iOS | CMake 3.29+, Xcode, iOS 13+ |
| Android | Android SDK, Android NDK, SDK Build Tools, Platform Tools, Ninja |

Linux packages:

```bash
sudo apt update
sudo apt-get install -y cmake pkg-config libdbus-1-dev libx11-dev libxkbcommon-x11-dev libx11-xcb-dev
```

Android expects these environment variables when building from the command line:

```powershell
ANDROID_HOME=C:\Users\<you>\AppData\Local\Android\Sdk
ANDROID_NDK_HOME=%ANDROID_HOME%\ndk\<version>
JAVA_HOME=<jdk-path>
```

## Building

### Desktop

Configure and build from the repository root:

```powershell
cmake -B build
cmake --build build --config Debug
```

Outputs are placed in:

```text
build/bin/Debug/
```

Run unit tests:

```powershell
build\bin\Debug\unittest.exe
```

### iOS

Generate an Xcode project and build the UIKit-hosted XCTest bundle for the simulator:

```bash
cmake -S . -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DCORE_BUILD_STATIC=ON \
  -DCORE_BUILD_UNITTEST=ON

cmake --build build-ios --target unittest --config Debug
```

Like Android, iOS configuration explicitly enables the supported static Core build. The `unittest` target builds `CoreUnitTestHost.app` and its `CoreUnitTests.xctest` bundle. Run the `unittest` scheme against a selected simulator from Xcode; automated simulator validation is still pending.

### Android

Android uses the NDK toolchain and writes final artifacts to the same output folder shape as desktop:

```text
build/bin/Debug/
```

Build Core as an Android static library:

```powershell
cmake -B build -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE="$env:ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-26 `
  -DCORE_BUILD_STATIC=ON `
  -DCORE_BUILD_UNITTEST=OFF `
  -DCORE_INSTALL=OFF `
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build --target core
```

An Android app repo should link Core into its own NativeActivity shared library. Core also generates a tiny `core.android.CoreNativeActivity` Java bridge and `core.android.CoreClipboardProvider` for Android framework features such as app/cache directories, window presentation, file dialogs, `content://` file access, clipboard bytes, and soft keyboard text input:

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

The app owns `ANativeActivity_onCreate`, the manifest, APK packaging, Java compilation, signing, install, and launch steps. Android and iOS use the same host order: initialize a `Platform_Window`, bind native objects with `platform_window_native_connect`, then start the app loop only for a new connection. Android reconnections preserve the existing app thread when an Activity is recreated. See [`docs/platform.md`](docs/platform.md) for the minimal entrypoint and packaging recipe.

## CMake Options

| Option | Default | Description |
|---|---|---|
| `CORE_BUILD_UNITTEST` | ON for root builds | Build unit tests |
| `CORE_INSTALL` | ON for root builds | Enable install target |
| `CORE_BUILD_UNITY` | OFF | Enable unity build |
| `CORE_BUILD_STATIC` | OFF | Build Core as a static library |