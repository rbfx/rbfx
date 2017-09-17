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

# Include common utilitles
include(UrhoCommon)

# Setup SDK install destinations
set (PATH_SUFFIX Urho3D)
if (WIN32)
    set (SCRIPT_EXT .bat)
    if (CMAKE_HOST_WIN32)
        set (PATH_SUFFIX .)
    endif ()
else ()
    set (SCRIPT_EXT .sh)
endif ()
if (ANDROID)
    # For Android platform, install to a path based on the chosen Android ABI, e.g. libs/armeabi-v7a
    set (LIB_SUFFIX s/${ANDROID_NDK_ABI_NAME})
elseif (URHO3D_64BIT)
    # Install to 'lib64' when one of these conditions is true
    if ((MINGW AND CMAKE_CROSSCOMPILING) OR URHO3D_USE_LIB64_RPM OR (HAS_LIB64 AND NOT URHO3D_USE_LIB_DEB))
        set (LIB_SUFFIX 64)
    endif ()
endif ()
set (DEST_INCLUDE_DIR include/Urho3D)    # The include directory path contains the 'Urho3D' suffix regardless of PATH_SUFFIX variable
set (DEST_BIN_DIR bin)
set (DEST_SHARE_DIR share/${PATH_SUFFIX})
set (DEST_BUNDLE_DIR ${DEST_SHARE_DIR}/Applications)
# Note to package maintainer: ${PATH_SUFFIX} for library could be removed if the extra path is not desired, but if so then the RPATH setting in Source's CMakeLists.txt needs to be adjusted accordingly
set (DEST_LIBRARY_DIR lib${LIB_SUFFIX}/${PATH_SUFFIX})
set (DEST_PKGCONFIG_DIR lib${LIB_SUFFIX}/pkgconfig)
set (DEST_THIRDPARTY_HEADERS ${DEST_INCLUDE_DIR}/ThirdParty)

# Install CMake modules and toolchains provided by and for Urho3D
install (DIRECTORY ${CMAKE_SOURCE_DIR}/CMake/ DESTINATION ${DEST_SHARE_DIR}/CMake)    # Note: the trailing slash is significant
# Install CMake build scripts
file (GLOB CMAKE_SCRIPTS ${CMAKE_SOURCE_DIR}/*${SCRIPT_EXT})
install (PROGRAMS ${CMAKE_SCRIPTS} DESTINATION ${DEST_SHARE_DIR}/Scripts)
# Install data files
install (DIRECTORY bin/Autoload bin/Data bin/CoreData DESTINATION ${DEST_SHARE_DIR}/Resources/)

option(URHO3D_ENABLE_ALL "Enables all optional subsystems (except database) by default" OFF)
option(URHO3D_IK "Inverse kinematics subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_NAVIGATION "Navigation subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_NETWORK "Networking subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_PHYSICS "Physics subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_URHO2D "2D subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_WEBP "WEBP support enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_PROFILING "Profiler support enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_TOOLS "Tools enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_SAMPLES "Build samples" ${URHO3D_ENABLE_ALL})

if (WIN32)
    option(URHO3D_D3D11 "Build with D3D11 renderer" ON)
    option(URHO3D_D3D9 "Build with D3D9 renderer" OFF)
    option(URHO3D_OPENGL "Build with OpenGL renderer" OFF)
else ()
    set(URHO3D_OPENGL ON)
endif ()

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Source)
