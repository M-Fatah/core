set(HEADER_FILES
    defer.h
    defines.h
    formatter.h
    hash.h
    json.h
    log.h
    reflect.h
    result.h
    source_location.h
    utils.h
    containers/array.h
    containers/hash_table.h
    containers/stack_array.h
    containers/string.h
    memory/memory.h
    memory/heap_allocator.h
    memory/arena_allocator.h
    memory/pool_allocator.h
    platform/platform.h
    serialization/binary_serializer.h
    serialization/json_serializer.h
    validate.h
)

set(SOURCE_FILES
    formatter.cpp
    json.cpp
    log.cpp
    memory/memory.cpp
    memory/heap_allocator.cpp
    memory/arena_allocator.cpp
    memory/pool_allocator.cpp
    validate.cpp
)

set(LIBS
)

if(WIN32)
    list(APPEND SOURCE_FILES
        platform/platform_win32.cpp
    )
    list(APPEND LIBS
        user32.lib
        winmm.lib
        dbghelp
    )
elseif(UNIX AND NOT APPLE)
    list(APPEND SOURCE_FILES
        platform/platform_linux.cpp
    )
    list(APPEND LIBS
        xcb
        X11
        X11-xcb
        dl
        pthread
    )
elseif(APPLE)
    list(APPEND SOURCE_FILES
        platform/platform_macos.mm
    )
    list(APPEND LIBS
        "-framework Cocoa"
        "-framework UniformTypeIdentifiers"
    )
endif()

set(BUILD_TYPE SHARED)
if(${CORE_BUILD_STATIC})
    set(BUILD_TYPE STATIC)
endif()
add_library(core ${BUILD_TYPE} ${HEADER_FILES} ${SOURCE_FILES})

# TODO:
target_sources(core
  PUBLIC
    FILE_SET all_my_modules TYPE CXX_MODULES
    BASE_DIRS
        ${PROJECT_SOURCE_DIR}
    FILES
        base64.cppm
        ecs.cppm
)

target_link_libraries(core PUBLIC ${LIBS})
target_compile_options(core PRIVATE ${GLOBAL_COMPILE_OPTIONS})
target_compile_definitions(core PRIVATE ${GLOBAL_COMPILE_DEFINITIONS})
# TODO: Put headers into its own folder?
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

set_target_properties(core
    PROPERTIES
        UNITY_BUILD_BATCH_SIZE 8
        UNITY_BUILD ${CORE_BUILD_UNITY}
)

include(GenerateExportHeader)
generate_export_header(core
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/export.h"
    INCLUDE_GUARD_NAME "CORE_API_H"
    EXPORT_MACRO_NAME "CORE_API"
    NO_EXPORT_MACRO_NAME "CORE_NO_API"
    DEPRECATED_MACRO_NAME "CORE_API_DEPRECATED"
    NO_DEPRECATED_MACRO_NAME "CORE_API_NO_DEPRECATED"
)