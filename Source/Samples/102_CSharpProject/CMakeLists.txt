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

# Emscripten does not support ucontext/setjmp/longjmp
if (NOT URHO3D_CSHARP)
    return ()
endif ()

if (MSVC)
    set (URHO3DNET_LIBRARY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}/$<CONFIG>/Urho3DNet.dll)
else ()
    set (URHO3DNET_LIBRARY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}/Urho3DNet.dll)
endif ()

# Managed executable must find Urho3DCSharp and Urho3DNet in the same directory, that is why it is put to DEST_BIN_DIR
# instead of samples directory.
add_target_csharp(102_CSharpProject /target:exe /reference:System.dll
    /reference:${URHO3DNET_LIBRARY} /out:${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}/102_CSharpProject.exe
    ${CMAKE_CURRENT_SOURCE_DIR}/Program.cs
)
add_dependencies(102_CSharpProject Urho3DNet)
