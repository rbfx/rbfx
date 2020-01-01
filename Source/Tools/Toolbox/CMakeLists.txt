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

if (NOT URHO3D_SYSTEMUI)
    return ()
endif ()

file (GLOB_RECURSE SOURCE_FILES *.cpp *.h *.hpp)
add_library (Toolbox ${SOURCE_FILES})
target_link_libraries (Toolbox PUBLIC Urho3D)
target_compile_definitions(Toolbox PRIVATE -DURHO3D_TOOLBOX_EXPORTS=1)
if (NOT BUILD_SHARED_LIBS)
    target_compile_definitions(Toolbox PUBLIC -DURHO3D_TOOLBOX_STATIC=1)
endif ()
target_include_directories(Toolbox
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install(TARGETS Toolbox EXPORT Urho3D
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h)
