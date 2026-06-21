# Core Android Smoke

Small NativeActivity smoke app for Core's Android platform backend. The app owns `ANativeActivity_onCreate` through the local `core_android_native_activity` object library; Core does not own an app entrypoint.

## Build

```powershell
cmake -S examples/android/smoke -B build/android-smoke `
  -DCMAKE_TOOLCHAIN_FILE=$env:ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake `
  -DANDROID_ABI=arm64-v8a `
  -DANDROID_PLATFORM=android-26 `
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/android-smoke --config Debug
```

The app library is written to `build/bin/Debug/libcore_android_smoke.so`.

## Package And Run

When `ANDROID_HOME` or `ANDROID_SDK_ROOT` points at an Android SDK with Build Tools installed, CMake also creates these targets. Packaging needs `aapt2`, `zipalign`, `apksigner`, and either `keytool` or an existing Android debug keystore. Install/run also need `adb`.

```powershell
cmake --build build/android-smoke --target core_android_smoke_apk
cmake --build build/android-smoke --target core_android_smoke_install
cmake --build build/android-smoke --target core_android_smoke_run
```

`core_android_smoke_apk` packages `AndroidManifest.xml`, `assets/hello.txt`, and `libcore_android_smoke.so` into a signed debug APK at `build/bin/Debug/core_android_smoke.apk`.

`core_android_smoke_run` installs the APK with `adb install -r` and launches `com.mfatah.core.smoke`.

This example intentionally has no Java/Kotlin, Gradle dependency, AndroidX, GameActivity, or `android_native_app_glue`.