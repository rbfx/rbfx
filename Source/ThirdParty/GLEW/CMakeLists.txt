#
# Copyright (c) 2008-2020 the Urho3D project.
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

file (GLOB_RECURSE SOURCE_FILES *.c)
add_library (GLEW STATIC ${SOURCE_FILES})
target_compile_definitions(GLEW PUBLIC -DGLEW_NO_GLU)
if (BUILD_SHARED_LIBS)
    target_compile_definitions(GLEW PRIVATE -DGLEW_BUILD)
else ()
    target_compile_definitions(GLEW PUBLIC  -DGLEW_STATIC)
endif ()
target_include_directories(GLEW
    SYSTEM BEFORE PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if (WIN32)
    target_link_libraries(GLEW opengl32.lib)
endif ()

if (NOT MINI_URHO)
    if (NOT URHO3D_MERGE_STATIC_LIBS)
        install(TARGETS GLEW EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    endif ()
endif ()
