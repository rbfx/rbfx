# Copyright (c) 2017-2022 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT URHO3D_TESTING)
    return ()
endif ()

include (../ThirdParty/catch2/Catch.cmake)

file (GLOB_RECURSE TEST_SOURCE_CODE RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp *.h)
list (REMOVE_ITEM TEST_SOURCE_CODE MainDll.cpp MainExe.cpp)

# Group source code in VS solution
group_sources()

set (TARGET_NAME Tests)

# Target which contains test code. It includes generated bindings code as well.
add_library(${TARGET_NAME}Lib SHARED MainDll.cpp ${TEST_SOURCE_CODE})
target_link_libraries(${TARGET_NAME}Lib PRIVATE Urho3D catch2)

# Executable test runner. Does not contain test code, only invokes actual main from the dll.
add_executable(${TARGET_NAME} MainExe.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE ${TARGET_NAME}Lib)
catch_discover_tests(${TARGET_NAME})

if (URHO3D_CSHARP)
    csharp_bind_target(
        TARGET ${TARGET_NAME}Lib
        SWIG ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/TestBindings.i
        CSPROJ ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Urho3DNet.Tests.csproj
        NAMESPACE Urho3DNet
        DEPENDS Urho3D
    )
    target_include_directories(${TARGET_NAME}Lib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
