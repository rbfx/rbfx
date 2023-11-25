#
# Copyright (c) 2017-2022 the rbfx project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

if (NOT URHO3D_TESTING)
    return ()
endif ()

include (../ThirdParty/catch2/Catch.cmake)

file (GLOB_RECURSE TEST_SOURCE_CODE RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp *.h)

# Group source code in VS solution
group_sources()

set (TARGET_NAME Tests)
add_executable(${TARGET_NAME} ${TEST_SOURCE_CODE})

target_link_libraries(${TARGET_NAME} PRIVATE Urho3D catch2)
catch_discover_tests(${TARGET_NAME})

if (URHO3D_CSHARP)
    add_target_csharp(
        TARGET Urho3DNet.Tests
        PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Urho3DNet.Tests.csproj
        OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Urho3DNet.Tests
        DEPENDS Urho3DNet
    )
endif()
