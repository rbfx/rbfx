cmake_minimum_required (VERSION 3.10)

project(Diligent-TestFramework)

set(SOURCE
    src/TempDirectory.cpp
    src/TestingEnvironment.cpp
)

set(INCLUDE
    include/TempDirectory.hpp
    include/TestingEnvironment.hpp
)

set(ALL_SOURCE ${SOURCE} ${INCLUDE})
add_library(Diligent-TestFramework STATIC ${ALL_SOURCE})
set_common_target_properties(Diligent-TestFramework)

set_target_properties(Diligent-TestFramework PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(Diligent-TestFramework
PRIVATE
    Diligent-BuildSettings
    Diligent-TargetPlatform
    Diligent-Common
PUBLIC
    gtest
)

target_include_directories(Diligent-TestFramework
PUBLIC
    include
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE})

set_target_properties(Diligent-TestFramework PROPERTIES
    FOLDER "DiligentCore/Tests"
)
