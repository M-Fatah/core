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
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>: -Werror -Wall -Wno-c++2b-extensions -Wno-enum-constexpr-conversion -Wno-c++11-narrowing -Wno-deprecated-declarations -Wno-invalid-offsetof>
    $<$<CXX_COMPILER_ID:GNU>: -Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>: /WX /W4>
)

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