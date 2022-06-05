set(HEADER_FILES
    assert.h
    defer.h
    defines.h
    ecs.h
    hash.h
    json.h
    logger.h
    result.h
    utils.h
    containers/array.h
    containers/string.h
    containers/hash_table.h
    memory/memory.h
    memory/heap_allocator.h
    memory/arena_allocator.h
    memory/pool_allocator.h
    platform/platform.h
    serialization/serializer.h
    serialization/binary_serializer.h
)

set(SOURCE_FILES
    assert.cpp
    ecs.cpp
    json.cpp
    logger.cpp
    memory/heap_allocator.cpp
    memory/arena_allocator.cpp
    memory/pool_allocator.cpp
    serialization/binary_serializer.cpp
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
endif()

add_library(core SHARED ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(core PRIVATE ${LIBS})
target_compile_options(core PRIVATE ${GLOBAL_COMPILE_OPTIONS})
# TODO: Put headers into its own folder.
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)

if(CORE_BUILD_UNITY)
    set_target_properties(core
        PROPERTIES
            UNITY_BUILD_BATCH_SIZE 8
            UNITY_BUILD true
    )
endif()

include(GenerateExportHeader)
generate_export_header(core
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/export.h"
    INCLUDE_GUARD_NAME "CORE_API_H"
    EXPORT_MACRO_NAME "CORE_API"
    NO_EXPORT_MACRO_NAME "CORE_NO_API"
    DEPRECATED_MACRO_NAME "CORE_API_DEPRECATED"
    NO_DEPRECATED_MACRO_NAME "CORE_API_NO_DEPRECATED"
)