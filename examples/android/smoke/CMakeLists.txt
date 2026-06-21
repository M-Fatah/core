cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    message(FATAL_ERROR "The Android smoke example requires the Android NDK toolchain file. Example: cmake -S examples/android/smoke -B build/android-smoke -G Ninja -DCMAKE_TOOLCHAIN_FILE=<ANDROID_NDK_HOME>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26")
endif()

project(core_android_smoke VERSION 0.0.1)

if(NOT ANDROID)
    message(FATAL_ERROR "The Android smoke example must be configured with the Android NDK toolchain file. Example: cmake -S examples/android/smoke -B build/android-smoke -G Ninja -DCMAKE_TOOLCHAIN_FILE=<ANDROID_NDK_HOME>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type." FORCE)
endif()

get_filename_component(CORE_ANDROID_SMOKE_BUILD_ROOT "${CMAKE_BINARY_DIR}/.." ABSOLUTE)
set(CORE_ANDROID_SMOKE_OUTPUT_DIR "${CORE_ANDROID_SMOKE_BUILD_ROOT}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CORE_ANDROID_SMOKE_OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CORE_ANDROID_SMOKE_OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CORE_ANDROID_SMOKE_OUTPUT_DIR}")

set(CORE_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(CORE_BUILD_TEST OFF CACHE BOOL "" FORCE)
set(CORE_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(CORE_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("../../.." "${CMAKE_CURRENT_BINARY_DIR}/core")

add_library(core_android_native_activity OBJECT
    src/android_native_activity_entry.cpp
)
target_link_libraries(core_android_native_activity PRIVATE core-options)

add_library(core_android_smoke SHARED
    src/android_smoke.cpp
    $<TARGET_OBJECTS:core_android_native_activity>
)
target_link_libraries(core_android_smoke PRIVATE core)
set_target_properties(core_android_smoke
    PROPERTIES
        OUTPUT_NAME "core_android_smoke"
        ARCHIVE_OUTPUT_DIRECTORY "${CORE_ANDROID_SMOKE_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CORE_ANDROID_SMOKE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CORE_ANDROID_SMOKE_OUTPUT_DIR}"
)

set(CORE_ANDROID_SMOKE_PACKAGE_NAME "com.mfatah.core.smoke")
set(CORE_ANDROID_SMOKE_MIN_SDK 26 CACHE STRING "Minimum Android SDK version for the smoke APK.")

set(CORE_ANDROID_SDK_ROOT "")
foreach(_core_android_sdk_root "$ENV{ANDROID_HOME}" "$ENV{ANDROID_SDK_ROOT}")
    if(_core_android_sdk_root AND EXISTS "${_core_android_sdk_root}")
        file(TO_CMAKE_PATH "${_core_android_sdk_root}" CORE_ANDROID_SDK_ROOT)
        break()
    endif()
endforeach()

if(CORE_ANDROID_SDK_ROOT)
    file(GLOB _core_android_build_tools LIST_DIRECTORIES true "${CORE_ANDROID_SDK_ROOT}/build-tools/*")
    list(SORT _core_android_build_tools COMPARE NATURAL ORDER DESCENDING)
    if(_core_android_build_tools)
        list(GET _core_android_build_tools 0 CORE_ANDROID_BUILD_TOOLS_DIR)
    endif()

    file(GLOB _core_android_platforms LIST_DIRECTORIES true "${CORE_ANDROID_SDK_ROOT}/platforms/android-*")
    list(SORT _core_android_platforms COMPARE NATURAL ORDER DESCENDING)
    foreach(_core_android_platform IN LISTS _core_android_platforms)
        if(EXISTS "${_core_android_platform}/android.jar")
            set(CORE_ANDROID_JAR "${_core_android_platform}/android.jar")
            break()
        endif()
    endforeach()

    find_program(CORE_ANDROID_AAPT2 NAMES aapt2 aapt2.exe PATHS "${CORE_ANDROID_BUILD_TOOLS_DIR}" NO_DEFAULT_PATH)
    find_program(CORE_ANDROID_ZIPALIGN NAMES zipalign zipalign.exe PATHS "${CORE_ANDROID_BUILD_TOOLS_DIR}" NO_DEFAULT_PATH)
    find_program(CORE_ANDROID_APKSIGNER NAMES apksigner apksigner.bat apksigner.exe PATHS "${CORE_ANDROID_BUILD_TOOLS_DIR}" NO_DEFAULT_PATH)
    find_program(CORE_ANDROID_ADB NAMES adb adb.exe PATHS "${CORE_ANDROID_SDK_ROOT}/platform-tools" NO_DEFAULT_PATH)
endif()
find_program(CORE_ANDROID_KEYTOOL NAMES keytool keytool.exe PATHS "$ENV{JAVA_HOME}/bin")

if(CORE_ANDROID_AAPT2 AND CORE_ANDROID_ZIPALIGN AND CORE_ANDROID_APKSIGNER AND CORE_ANDROID_JAR)
    set(CORE_ANDROID_SMOKE_APK_DIR "${CORE_ANDROID_SMOKE_OUTPUT_DIR}")
    set(CORE_ANDROID_SMOKE_APK_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}/apk")
    set(CORE_ANDROID_SMOKE_AAPT_APK "${CORE_ANDROID_SMOKE_APK_DIR}/core_android_smoke-aapt.apk")
    set(CORE_ANDROID_SMOKE_PACKED_APK "${CORE_ANDROID_SMOKE_APK_DIR}/core_android_smoke-packed.apk")
    set(CORE_ANDROID_SMOKE_ALIGNED_APK "${CORE_ANDROID_SMOKE_APK_DIR}/core_android_smoke-aligned.apk")
    set(CORE_ANDROID_SMOKE_APK "${CORE_ANDROID_SMOKE_APK_DIR}/core_android_smoke.apk")
    set(CORE_ANDROID_SMOKE_KEYSTORE "${CORE_ANDROID_SMOKE_APK_WORK_DIR}/debug.keystore")

    file(GLOB_RECURSE CORE_ANDROID_SMOKE_ASSETS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")

    add_custom_command(
        OUTPUT "${CORE_ANDROID_SMOKE_AAPT_APK}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CORE_ANDROID_SMOKE_APK_DIR}"
        COMMAND "${CORE_ANDROID_AAPT2}" link
            -o "${CORE_ANDROID_SMOKE_AAPT_APK}"
            -I "${CORE_ANDROID_JAR}"
            --manifest "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
            -A "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            --min-sdk-version "${CORE_ANDROID_SMOKE_MIN_SDK}"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
            ${CORE_ANDROID_SMOKE_ASSETS}
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${CORE_ANDROID_SMOKE_PACKED_APK}"
        COMMAND "${CMAKE_COMMAND}"
            "-DINPUT_APK=${CORE_ANDROID_SMOKE_AAPT_APK}"
            "-DNATIVE_LIBRARY=$<TARGET_FILE:core_android_smoke>"
            "-DANDROID_ABI=${ANDROID_ABI}"
            "-DWORK_DIR=${CORE_ANDROID_SMOKE_APK_WORK_DIR}/stage"
            "-DOUTPUT_APK=${CORE_ANDROID_SMOKE_PACKED_APK}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/package_apk.cmake"
        DEPENDS
            core_android_smoke
            "${CORE_ANDROID_SMOKE_AAPT_APK}"
            "${CMAKE_CURRENT_SOURCE_DIR}/package_apk.cmake"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${CORE_ANDROID_SMOKE_ALIGNED_APK}"
        COMMAND "${CORE_ANDROID_ZIPALIGN}" -P 16 -f -v 4 "${CORE_ANDROID_SMOKE_PACKED_APK}" "${CORE_ANDROID_SMOKE_ALIGNED_APK}"
        DEPENDS "${CORE_ANDROID_SMOKE_PACKED_APK}"
        VERBATIM
    )

    set(_core_android_smoke_keystore_depends "")
    set(_core_android_smoke_keystore_path "${CORE_ANDROID_SMOKE_KEYSTORE}")
    if(CORE_ANDROID_KEYTOOL)
        add_custom_command(
            OUTPUT "${CORE_ANDROID_SMOKE_KEYSTORE}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CORE_ANDROID_SMOKE_APK_WORK_DIR}"
            COMMAND "${CORE_ANDROID_KEYTOOL}" -genkeypair -v
                -keystore "${CORE_ANDROID_SMOKE_KEYSTORE}"
                -storepass android
                -alias androiddebugkey
                -keypass android
                -keyalg RSA
                -keysize 2048
                -validity 10000
                -dname "CN=Android Debug,O=Android,C=US"
            VERBATIM
        )
        list(APPEND _core_android_smoke_keystore_depends "${CORE_ANDROID_SMOKE_KEYSTORE}")
    else()
        set(_core_android_smoke_keystore_path "")
        foreach(_core_android_debug_keystore "$ENV{USERPROFILE}/.android/debug.keystore" "$ENV{HOME}/.android/debug.keystore")
            if(_core_android_debug_keystore AND EXISTS "${_core_android_debug_keystore}")
                file(TO_CMAKE_PATH "${_core_android_debug_keystore}" _core_android_smoke_keystore_path)
                break()
            endif()
        endforeach()
    endif()

    if(_core_android_smoke_keystore_path)
        add_custom_command(
            OUTPUT "${CORE_ANDROID_SMOKE_APK}"
            COMMAND "${CORE_ANDROID_APKSIGNER}" sign
                --ks "${_core_android_smoke_keystore_path}"
                --ks-pass pass:android
                --key-pass pass:android
                --ks-key-alias androiddebugkey
                --out "${CORE_ANDROID_SMOKE_APK}"
                "${CORE_ANDROID_SMOKE_ALIGNED_APK}"
            DEPENDS
                "${CORE_ANDROID_SMOKE_ALIGNED_APK}"
                ${_core_android_smoke_keystore_depends}
            VERBATIM
        )

        add_custom_target(core_android_smoke_apk DEPENDS "${CORE_ANDROID_SMOKE_APK}")

        if(CORE_ANDROID_ADB)
            add_custom_target(core_android_smoke_install
                COMMAND "${CORE_ANDROID_ADB}" install -r "${CORE_ANDROID_SMOKE_APK}"
                DEPENDS core_android_smoke_apk
                VERBATIM
            )

            add_custom_target(core_android_smoke_run
                COMMAND "${CORE_ANDROID_ADB}" shell monkey -p "${CORE_ANDROID_SMOKE_PACKAGE_NAME}" 1
                DEPENDS core_android_smoke_install
                VERBATIM
            )
        endif()
    else()
        message(WARNING "core_android_smoke_apk target is disabled because no keytool or Android debug keystore was found.")
    endif()
else()
    message(WARNING "core_android_smoke_apk target is disabled because Android SDK Build Tools were not found. Set ANDROID_HOME or ANDROID_SDK_ROOT.")
endif()