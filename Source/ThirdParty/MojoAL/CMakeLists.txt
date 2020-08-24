#
# Copyright (c) 2017-2020 the rbfx project.
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

file (GLOB_RECURSE SOURCE_FILES *.c *.h)
add_library (MojoAL STATIC ${SOURCE_FILES})
target_include_directories(MojoAL PRIVATE include)
target_include_directories(MojoAL PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/AL>
    $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/AL>      # For compatibility with official OpenAL sdk on all platforms.
)
target_link_libraries(MojoAL PRIVATE SDL)
if (BUILD_SHARED_LIBS)
    target_compile_definitions(MojoAL PUBLIC -DAL_IMPORTS PRIVATE -DAL_EXPORTS)
else ()
    target_compile_definitions(MojoAL PUBLIC -DAL_LIBTYPE_STATIC)
endif ()

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS MojoAL EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

