set(HEADER_FILES
)

set(SOURCE_FILES
    src/unittest.cpp
    src/unittest_core.cpp
    src/unittest_formatter.cpp
    src/unittest_containers.cpp
    src/unittest_platform.cpp
    src/unittest_reflect.cpp
    src/unittest_serializer.cpp
)

set(LIBS
    core
)

add_executable(unittest ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(unittest PRIVATE ${LIBS})
set_target_properties(unittest
    PROPERTIES
        UNITY_BUILD_BATCH_SIZE 8
        UNITY_BUILD ${CORE_BUILD_UNITY}
)