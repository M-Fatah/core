cmake_minimum_required(VERSION 3.20)

project(core VERSION 0.0.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CORE_IS_MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CORE_IS_MAIN_PROJECT ON)
endif()

option(CORE_BUILD_UNITTEST        "Enable building unittest." ${CORE_IS_MAIN_PROJECT})
option(CORE_INSTALL               "Install core library."     ${CORE_IS_MAIN_PROJECT})
option(CORE_BUILD_UNITY           "Enable unity build."       OFF)

set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/bin/"         )
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")

# Global compile definitions.
add_compile_definitions(
    $<$<CONFIG:DEBUG>:DEBUG=1>
    $<$<CONFIG:RELWITHDEBINFO>:DEBUG=1>
    $<$<CONFIG:RELEASE>:RELEASE=1>
    $<$<CONFIG:MINSIZEREL>:RELEASE=1>
)

# Treat all compiler warnings as errors and increase warning level.
if(MSVC)
    set(GLOBAL_COMPILE_OPTIONS -W4 -WX -std:c++20)
else()
    set(GLOBAL_COMPILE_OPTIONS -Wall -Wextra -pedantic -Werror -std=c++20)
endif()

add_subdirectory(core)

if (CORE_BUILD_UNITTEST)
    message(STATUS "Build unittest flag is enabled.")
    add_subdirectory(unittest)
endif(CORE_BUILD_UNITTEST)

if(CORE_BUILD_UNITY)
    message(STATUS "Unity build flag is enabled.")
endif(CORE_BUILD_UNITY)

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
endif(CORE_INSTALL)