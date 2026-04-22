set(HEADER_FILES
)

set(SOURCE_FILES
    src/unittest.cpp
    src/unittest_core.cpp
    src/unittest_formatter.cpp
    src/unittest_containers.cpp
    src/unittest_math.cpp
    src/unittest_platform.cpp
    src/unittest_reflect.cpp
    src/unittest_serializer.cpp
)

set(LIBS
    core
)

add_executable(core_unittest ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(core_unittest PRIVATE ${LIBS})
set_target_properties(core_unittest
    PROPERTIES
        UNITY_BUILD_BATCH_SIZE 8
        UNITY_BUILD ${CORE_BUILD_UNITY}
)

# Math micro-benchmark — not a test, run manually to compare SIMD vs scalar paths.
add_executable(bench_math src/bench_math.cpp)
target_link_libraries(bench_math PRIVATE ${LIBS})