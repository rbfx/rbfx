#
# Copyright (c) 2017-2019 Rokas Kupstys.
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

if (NOT URHO3D_SYSTEMUI)
    return ()
endif ()

file (GLOB_RECURSE SOURCE_FILES *.cpp *.h *.hpp)

if (URHO3D_CSHARP AND URHO3D_PLUGINS)
    # Use C# executable to host .net runtime and load editor.
    set (CMAKE_SHARED_LIBRARY_PREFIX lib)
    add_library (Editor SHARED ${SOURCE_FILES})
    add_target_csharp(
        TARGET EditorHost
        PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/EditorHost/EditorHost.csproj
        OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Editor.exe
        DEPENDS Urho3DNet)
else ()
    # No C# or pugins support, editor can be a true native executable.
    if (NOT URHO3D_WIN32_CONSOLE)
        set (TARGET_TYPE WIN32)
    endif ()
    add_executable (Editor ${TARGET_TYPE} ${SOURCE_FILES})
endif ()
target_link_libraries (Editor Toolbox Urho3D)
if (TARGET tracy-embedded-profiler)
    target_link_libraries (Editor tracy-embedded-profiler)
endif ()
if (URHO3D_PLUGINS)
    target_link_libraries (Editor PlayerCommon)
endif ()
install(TARGETS Editor
    RUNTIME DESTINATION ${DEST_TOOLS_DIR}
    LIBRARY DESTINATION ${DEST_TOOLS_DIR}
)
