# External dependencies.
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.8
    GIT_PROGRESS   TRUE
)
set(DOCTEST_WITH_TESTS OFF)
set(DOCTEST_NO_INSTALL ON)
FetchContent_MakeAvailable(doctest)

set(HEADER_FILES
)

set(SOURCE_FILES
    src/unittest.cpp
    src/unittest_core.cpp
    src/unittest_containers.cpp
    src/unittest_platform.cpp
    src/unittest_reflect.cpp
)

set(LIBS
    core
    doctest
)

add_executable(unittest ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(unittest ${LIBS})
target_compile_options(unittest PRIVATE ${GLOBAL_COMPILE_OPTIONS})
set_target_properties(unittest
    PROPERTIES
        UNITY_BUILD_BATCH_SIZE 8
        UNITY_BUILD ${CORE_BUILD_UNITY}
)