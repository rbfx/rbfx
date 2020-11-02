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

# Workaround for MinGW 6.1.0 and above where it throws ICE (internal compilation error) when -O3 is used
if (MINGW AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.1.0 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 6.1.0))  # 6.1.0 is the last known bad version
    string (REPLACE -O3 -O2 CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif ()

# Define source files
file (GLOB_RECURSE SOURCE_FILES *.h *.cpp)
set (SOURCE_FILES ${SOURCE_FILES})
add_library (Bullet STATIC ${SOURCE_FILES})
target_include_directories(Bullet SYSTEM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Bullet>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
           $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/Bullet>
)

if (URHO3D_SSE)
    target_compile_definitions(Bullet PUBLIC -DBT_USE_SSE=1)
endif ()

if (NOT MINI_URHO)
    install(DIRECTORY Bullet DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR} FILES_MATCHING PATTERN *.h)
    if (NOT URHO3D_MERGE_STATIC_LIBS)
        install(TARGETS Bullet EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    endif ()
endif ()
