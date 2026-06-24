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
| `core/containers/` | Array, string, span, ring buffer, hash table, hash set, stack array |
| `core/math/` | Scalar helpers, vectors, matrices, quaternion, random, NEON / AVX / scalar paths |
| `core/formatter.h` | Type-safe formatting with Core strings and math types |
| `core/print.h`, `core/log.h` | Colored printing and log helpers |
| `core/defer.h` | Scope-exit cleanup macro |
| `core/validate.h` | Debug runtime validation with source locations |
| `core/tester.h` | Small unit-test framework |
| `core/result.h` | Result/error-returning pattern |
| `core/hash.h` | FNV-32 and type-generic hashing |
| `core/reflect.h` | Compile-time type reflection helpers |
| `core/serialization/` | Binary and JSON serializers |
| `core/ecs.h` | Minimal ECS |
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
| Linux | X11/XCB |
| macOS | Cocoa |
| Android | NDK NativeActivity |

Android support is NDK-only: no GameActivity, AndroidX, Jetpack, Gradle dependency, Java app layer, or `android_native_app_glue`.

## Prerequisites

| Platform | Requirements |
|---|---|
| Windows | CMake 3.25+ |
| Linux | CMake 3.25+, X11/XCB development packages |
| macOS | CMake 3.25+, Xcode Command Line Tools |
| Android | Android SDK, Android NDK, SDK Build Tools, Platform Tools, Ninja |

Linux packages:

```bash
sudo apt update
sudo apt-get install -y cmake libx11-dev libxkbcommon-x11-dev libx11-xcb-dev
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
  -DCORE_BUILD_TEST=OFF `
  -DCORE_BUILD_UNITTEST=OFF `
  -DCORE_INSTALL=OFF `
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build --target core
```

An Android app repo should link Core into its own NativeActivity shared library:

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

The app owns `ANativeActivity_onCreate`, the manifest, APK packaging, signing, install, and launch steps. See [`docs/platform.md`](docs/platform.md) for the minimal entrypoint and packaging recipe.

## CMake Options

| Option | Default | Description |
|---|---|---|
| `CORE_BUILD_UNITTEST` | ON for root desktop builds, OFF on Android | Build unit tests |
| `CORE_BUILD_TEST` | ON for root desktop builds, OFF on Android | Build sample test executable |
| `CORE_INSTALL` | ON for root builds | Enable install target |
| `CORE_BUILD_UNITY` | OFF | Enable unity build |
| `CORE_BUILD_STATIC` | OFF | Build Core as a static library |