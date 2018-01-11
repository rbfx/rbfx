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

# Urho3D tools
set (CMAKE_INSTALL_RPATH "../lib${LIB_SUFFIX}")

if (URHO3D_PACKAGING OR         # When packing resources
    URHO3D_PACKAGING_TOOL OR    # When crosscompiling and tool is required
    URHO3D_TOOLS)
    add_subdirectory (PackageTool)
endif ()
if (URHO3D_TOOLS AND DESKTOP)
    add_subdirectory (Toolbox)
    add_subdirectory (AssetImporter)
    add_subdirectory (AssetViewer)
    add_subdirectory (OgreImporter)
    add_subdirectory (RampGenerator)
    add_subdirectory (SpritePacker)
    add_subdirectory (Editor)
endif ()

file(GLOB tool_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach (tool ${tool_dirs})
    if (TARGET ${tool})
        set_target_properties(${tool} PROPERTIES FOLDER Tools)
    endif ()
endforeach ()
