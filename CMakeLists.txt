cmake_minimum_required(VERSION 3.28.2)

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
set(PREBUILT_MODULE_PATH           "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

# Global compile definitions.
set(GLOBAL_COMPILE_DEFINITIONS
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

set(CMAKE_CXX_STANDARD 20)

# Global compile options.
if(MSVC)
    set(GLOBAL_COMPILE_OPTIONS -W4 -WX -std:c++20 -Zc:preprocessor -wd5105 -wd4273)
else()
    # TODO: -Wno-deprecated-declarations is due to doctest.h sprintf() function.
    set(GLOBAL_COMPILE_OPTIONS -Wall -Wextra -pedantic -Werror -std=c++20 -fno-exceptions -Wno-invalid-offsetof -Wno-nested-anon-types -Wno-c++11-narrowing -Wno-gnu-zero-variadic-macro-arguments -Wno-deprecated-declarations -Wno-enum-constexpr-conversion -Wno-unknown-warning-option
    )
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