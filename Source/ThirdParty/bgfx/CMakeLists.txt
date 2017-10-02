#
# Copyright (c) 2008-2017 the Urho3D project.
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

set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOLEAN "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOLEAN "" FORCE)
add_subdirectory(3rdparty/glslang)
add_subdirectory(3rdparty/glsl-optimizer)

file (GLOB_RECURSE SOURCE_FILES
    src/*.cpp
    src/*.h
    include/*.h
    3rdparty/fcpp/cpp*.c
    3rdparty/fcpp/*.h
    3rdparty/forsyth-too/*.c
    3rdparty/forsyth-too/*.h
    3rdparty/ib-compress/*.cpp
    3rdparty/ib-compress/*.h
)
list (REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/amalgamated.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/amalgamated.mm)
add_library(bgfx STATIC ${SOURCE_FILES})
target_link_libraries(bgfx bimg glslang glsl_optimizer)
target_include_directories(bgfx PUBLIC include
    PRIVATE
    3rdparty
    3rdparty/dxsdk/include
    3rdparty/freetype
    3rdparty/khronos
)

install (DIRECTORY include/bgfx/ DESTINATION ${DEST_THIRDPARTY_HEADERS}/bgfx FILES_MATCHING PATTERN *.h)
install_to_build_tree(bgfx)
