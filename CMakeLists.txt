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

# Set CMake minimum version and CMake policy required by UrhoCommon module
cmake_minimum_required (VERSION 3.2.3)

# Set project name
project (Urho3D)

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules)

# Include file that sets up all configurable properties
include(UrhoOptions)

# Include common utilitles
include(UrhoCommon)

# Enable common build options
set (CMAKE_CXX_STANDARD 11)
if (EMSCRIPTEN)
    # Workaround until emscripten supports CMAKE_CXX_STANDARD
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
endif ()

set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (URHO3D_SSE AND NOT MSVC)
    # Build engine and it's dependencies with SSE/SSE2 enabled.
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")
endif ()

if (URHO3D_STATIC_RUNTIME)
    ucm_set_runtime(STATIC)
else ()
    ucm_set_runtime(DYNAMIC)
endif ()

add_subdirectory(Source)

include(UrhoPackaging)

# Install CMake modules and toolchains provided by and for Urho3D
install (DIRECTORY ${CMAKE_SOURCE_DIR}/CMake/ DESTINATION ${DEST_SHARE_DIR}/CMake)    # Note: the trailing slash is significant
# Install CMake build scripts
file (GLOB CMAKE_SCRIPTS ${CMAKE_SOURCE_DIR}/*${SCRIPT_EXT})
install (PROGRAMS ${CMAKE_SCRIPTS} DESTINATION ${DEST_SHARE_DIR}/Scripts)
# Install data files
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach (RES_DIR ${RESOURCE_DIRS})
    get_filename_component (NAME ${RES_DIR} NAME)
    install (DIRECTORY ${RES_DIR} DESTINATION ${DEST_RESOURCE_DIR})
    if (NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME})
        create_symlink(${RES_DIR} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${NAME})
    endif ()
endforeach ()


# Print current build configuration
message(STATUS "Urho3D Configuration:")
if (URHO3D_STATIC_RUNTIME)
    message(STATUS "  Runtime         STATIC")
else ()
    message(STATUS "  Runtime         SHARED")
endif ()
message(STATUS "  Library         ${URHO3D_LIBRARY_TYPE}")
message(STATUS "  SSE             ${URHO3D_SSE}")
message(STATUS "  2D              ${URHO3D_URHO2D}")
message(STATUS "  IK              ${URHO3D_IK}")
message(STATUS "  Threading       ${URHO3D_THREADING}")
message(STATUS "  Navigation      ${URHO3D_NAVIGATION}")
message(STATUS "  Network         ${URHO3D_NETWORK}")
message(STATUS "  Physics         ${URHO3D_PHYSICS}")
message(STATUS "  Samples         ${URHO3D_SAMPLES}")
message(STATUS "  WebP            ${URHO3D_WEBP}")
message(STATUS "Developer options:")
message(STATUS "  Packaging       ${URHO3D_PACKAGING}")
message(STATUS "  SystemUI        ${URHO3D_SYSTEMUI}")
message(STATUS "  Logging         ${URHO3D_LOGGING}")
message(STATUS "  Profiling       ${URHO3D_PROFILING}")
message(STATUS "  Extras          ${URHO3D_EXTRAS}")
message(STATUS "  Tools           ${URHO3D_TOOLS}")
if (TARGET profiler_gui)
    message(STATUS "     Profiler GUI ${URHO3D_PROFILING}")
endif ()
