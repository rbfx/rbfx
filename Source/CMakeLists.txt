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

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # Enable parallel builds
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif ()

add_subdirectory(ThirdParty)

if (URHO3D_CSHARP)
	add_subdirectory(CSharp)
endif ()

add_subdirectory (Urho3D)

if (NOT CMAKE_CROSS_COMPILING AND URHO3D_TOOLS)
    add_subdirectory (Tools)
    if (URHO3D_EXTRAS)
        add_subdirectory (Extras)
    endif ()
endif ()

if (URHO3D_SAMPLES)
    add_subdirectory (Samples)
endif ()

install(EXPORT Urho3D DESTINATION ${DEST_SHARE_DIR}/CMake)
