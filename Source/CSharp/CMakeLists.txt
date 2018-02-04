#
# Copyright (c) 2008-2018 the Urho3D project.
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

add_subdirectory(Generator)

file (GLOB_RECURSE ${Urho3D_SOURCE_DIR}/Source/Urho3D *.h)
add_custom_target(CSharpGenerateBindings
    COMMAND $<TARGET_FILE:CSharpGenerator>
        ${CMAKE_CURRENT_SOURCE_DIR}/Rules.xml
        ${Urho3D_SOURCE_DIR}/Source/Urho3D
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES ${GLOB_RECURSE}
    DEPENDS CSharpGenerator
)

file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/CApi.cpp "")
add_library(Urho3DCSharp SHARED CSharp.h CSharp.cpp ${CMAKE_CURRENT_BINARY_DIR}/CApi.cpp)
target_link_libraries(Urho3DCSharp Urho3D)
target_include_directories(Urho3DCSharp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(Urho3DCSharp CSharpGenerateBindings)
