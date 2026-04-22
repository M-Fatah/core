cmake_minimum_required(VERSION 3.20)

project(core VERSION 0.0.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CORE_IS_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CORE_IS_MAIN_PROJECT ON)
endif()

option(CORE_BUILD_UNITTEST "Enable building unittest." ${CORE_IS_MAIN_PROJECT})
option(CORE_INSTALL        "Enable installing core."   ${CORE_IS_MAIN_PROJECT})
option(CORE_BUILD_UNITY    "Enable unity build."       OFF)
option(CORE_BUILD_STATIC   "Enable static build."      OFF)

set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

add_library(core-options INTERFACE)
target_compile_features(core-options INTERFACE cxx_std_20)
target_compile_definitions(core-options INTERFACE
    $<$<CONFIG:DEBUG>:DEBUG=1>
    $<$<CONFIG:RELWITHDEBINFO>:DEBUG=1>
    $<$<CONFIG:RELEASE>:RELEASE=1>
    $<$<CONFIG:MINSIZEREL>:RELEASE=1>
    $<$<CXX_COMPILER_ID:Clang>:COMPILER_CLANG=1>
    $<$<CXX_COMPILER_ID:AppleClang>:COMPILER_CLANG=1>
    $<$<CXX_COMPILER_ID:GNU>:COMPILER_GCC=1>
    $<$<CXX_COMPILER_ID:MSVC>:COMPILER_MSVC=1>
    $<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS=1>
    $<$<PLATFORM_ID:Linux>:PLATFORM_LINUX=1>
    $<$<PLATFORM_ID:Darwin>:PLATFORM_MACOS=1>
)
target_compile_options(core-options INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>: -Werror -Wall -Wno-c++2b-extensions -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-invalid-offsetof -Wno-missing-braces>
    $<$<CXX_COMPILER_ID:GNU>: -Werror -Wall -Wextra -std=c++2b -Wno-invalid-offsetof -Wno-missing-field-initializers>
    $<$<CXX_COMPILER_ID:MSVC>: -WX -W4 -Zc:preprocessor -wd4201>
)
target_include_directories(core-options INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)

# SIMD arch gates — compile-time selection per target architecture.
# ARM64 (Apple Silicon, Linux/Windows ARM): NEON, always available on the ISA.
# x86_64 (Windows/Linux): AVX, Sandy Bridge (2011+) required. Anything beyond AVX
# (AVX2, AVX-512) is not guaranteed and stays out of scope.
#
# `CORE_SIMD_FORCE_SCALAR` overrides arch detection for parity testing — the
# arch flags are not emitted when it's on, so every math op falls through to its
# scalar branch.
option(CORE_SIMD_FORCE_SCALAR "Force scalar SIMD fallback for parity testing." OFF)
if(CORE_SIMD_FORCE_SCALAR)
    target_compile_definitions(core-options INTERFACE SIMD_FORCE_SCALAR=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    target_compile_definitions(core-options INTERFACE SIMD_NEON=1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|x64)$")
    target_compile_definitions(core-options INTERFACE SIMD_AVX=1)
    if(MSVC)
        target_compile_options(core-options INTERFACE /arch:AVX)
    else()
        target_compile_options(core-options INTERFACE -mavx)
    endif()
endif()

add_subdirectory(core)
add_subdirectory(test)

if (CORE_BUILD_UNITTEST)
    message(STATUS "Build unittest flag is enabled.")
    add_subdirectory(unittest)
endif()

if(CORE_BUILD_UNITY)
    message(STATUS "Unity build flag is enabled.")
endif()

# Package and archive.
# TODO: Add include directories to install.
if(CORE_INSTALL)
    set(INSTALL_TARGETS core)
    set(INSTALL_NAME ${PROJECT_NAME}_v${PROJECT_VERSION})
    set(INSTALL_DIR ${CMAKE_SOURCE_DIR}/packages/${INSTALL_NAME})
    set(CMAKE_INSTALL_PREFIX ${INSTALL_DIR})

    install(CODE "file(REMOVE_RECURSE ${INSTALL_DIR})")
    install(TARGETS ${INSTALL_TARGETS} RUNTIME DESTINATION ".")
    install(CODE
        "message(STATUS \"Archiving package ${INSTALL_NAME}\")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar \"cvf\" \"${INSTALL_DIR}.zip\" --format=zip .
            WORKING_DIRECTORY \"${INSTALL_DIR}\"
            OUTPUT_QUIET
        )"
    )
endif()