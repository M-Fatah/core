set(HEADER_FILES
)

set(SOURCE_FILES
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

if(IOS)
    if(NOT XCODE)
        message(FATAL_ERROR "iOS unit tests require the Xcode generator.")
    endif()

    find_package(XCTest REQUIRED)

    set(IOS_RESOURCE_FILES
        ios/resources/platform_resource.txt
    )
    set_source_files_properties(${IOS_RESOURCE_FILES}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources
    )

    add_executable(unittest_ios_host MACOSX_BUNDLE
        ios/unittest_ios_host.mm
        ${IOS_RESOURCE_FILES}
    )
    target_link_libraries(unittest_ios_host PRIVATE
        ${LIBS}
        "-framework UIKit"
    )
    set_target_properties(unittest_ios_host
        PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME "CoreUnitTestHost"
            MACOSX_BUNDLE_BUNDLE_VERSION "1"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.mfatah.core.unittest.host"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            OUTPUT_NAME "CoreUnitTestHost"
            XCODE_GENERATE_SCHEME TRUE
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.mfatah.core.unittest.host"
            XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    )

    xctest_add_bundle(unittest unittest_ios_host
        ios/unittest_ios.mm
        ${SOURCE_FILES}
    )
    target_link_libraries(unittest PRIVATE
        ${LIBS}
        "-framework Metal"
        "-framework QuartzCore"
    )
    set_target_properties(unittest
        PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME "CoreUnitTests"
            MACOSX_BUNDLE_BUNDLE_VERSION "1"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.mfatah.core.unittest"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
            OUTPUT_NAME "CoreUnitTests"
            UNITY_BUILD_BATCH_SIZE 8
            UNITY_BUILD ${CORE_BUILD_UNITY}
            XCODE_GENERATE_SCHEME TRUE
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.mfatah.core.unittest"
            XCODE_ATTRIBUTE_SKIP_INSTALL "YES"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    )
else()
    add_executable(unittest
        ${HEADER_FILES}
        src/unittest.cpp
        ${SOURCE_FILES}
    )
    target_link_libraries(unittest PRIVATE ${LIBS})
    set_target_properties(unittest
        PROPERTIES
            UNITY_BUILD_BATCH_SIZE 8
            UNITY_BUILD ${CORE_BUILD_UNITY}
    )
endif()