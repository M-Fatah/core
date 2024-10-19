set(HEADER_FILES
)

set(SOURCE_FILES
    src/test.cpp
)

set(LIBS
    core
)

add_executable(test ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(test ${LIBS})
target_compile_options(test PRIVATE ${GLOBAL_COMPILE_OPTIONS})
target_compile_definitions(test PRIVATE ${GLOBAL_COMPILE_DEFINITIONS})
set_target_properties(test
    PROPERTIES
        UNITY_BUILD_BATCH_SIZE 8
        UNITY_BUILD ${CORE_BUILD_UNITY}
)