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

file (GLOB_RECURSE SOURCE_FILES
    src/*.cpp
    src/*.h
    src/*.mm
    include/*.h
)
list (REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/amalgamated.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/amalgamated.mm)
add_library(bx STATIC ${SOURCE_FILES})
set (PUBLIC_INCLUDES include)
if (MSVC) 
    list (APPEND PUBLIC_INCLUDES include/compat/msvc)
endif ()
target_include_directories(bx PUBLIC ${PUBLIC_INCLUDES} PRIVATE 3rdparty/catch)
if (MSVC)
    target_compile_definitions( bx PUBLIC "__STDC_FORMAT_MACROS" )
endif ()

install (DIRECTORY include/bx/ DESTINATION ${DEST_THIRDPARTY_HEADERS}/bx FILES_MATCHING PATTERN *.h)
install (DIRECTORY include/bx/ DESTINATION ${DEST_THIRDPARTY_HEADERS}/bx FILES_MATCHING PATTERN *.inl)
install_to_build_tree(bx)
