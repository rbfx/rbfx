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

set (URHO3D_URL "https://github.com/urho3d/Urho3D")
set (URHO3D_DESCRIPTION "Urho3D is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE (http://www.ogre3d.org) and Horde3D (http://www.horde3d.org).")
execute_process (COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE URHO3D_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
string (REGEX MATCH "([^.]+)\\.([^.]+)\\.(.+)" MATCHED "${URHO3D_VERSION}")

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
if (WIN32)
    set (DEST_RESOURCE_DIR ${DEST_BIN_DIR})
else ()
    set (DEST_RESOURCE_DIR ${DEST_SHARE_DIR}/Resources)
endif ()
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
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach (RES_DIR Autoload Data CoreData)
    install (DIRECTORY bin/${RES_DIR} DESTINATION ${DEST_RESOURCE_DIR})
    if (NOT EXISTS ${CMAKE_BINARY_DIR}/bin/${RES_DIR})
        create_symlink(bin/${RES_DIR} ${CMAKE_BINARY_DIR}/bin/${RES_DIR})
    endif ()
endforeach ()

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
option(URHO3D_STATIC_RUNTIME "Enable link to static runtime" OFF)
option(URHO3D_LOGGING "Enable logging subsystem" ON)
option(URHO3D_EXTRAS "Build extra tools" ${URHO3D_ENABLE_ALL})
option(URHO3D_SYSTEMUI "Build SystemUI subsystem" ${URHO3D_ENABLE_ALL})
option(URHO3D_SSE "Enable SSE instructions" ON)

if (WIN32)
    set(URHO3D_RENDERER D3D11 CACHE STRING "Select renderer: D3D9 | D3D11 | OpenGL")
    if (MSVC)
        option(URHO3D_MINIDUMPS "Enable writing minidumps on crash" ${URHO3D_ENABLE_ALL})
    endif ()
else ()
    set(URHO3D_RENDERER OpenGL)
endif ()

string(TOUPPER "${URHO3D_RENDERER}" URHO3D_RENDERER)
if (URHO3D_RENDERER STREQUAL OPENGL)
    set (URHO3D_OPENGL ON)
elseif (URHO3D_RENDERER STREQUAL D3D9)
    set (URHO3D_D3D9 ON)
elseif (URHO3D_RENDERER STREQUAL D3D11)
    set (URHO3D_D3D11 ON)
endif ()

if (URHO3D_STATIC_RUNTIME)
    ucm_set_runtime(STATIC)
else ()
    ucm_set_runtime(DYNAMIC)
endif ()

# Enable common build options
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (CMAKE_CROSSCOMPILING AND MINGW)
    # Symlink windows libraries and headers to appease some libraries that do not use all-lowercase names and break on
    # case-sensitive file systems.
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/workaround)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/windows.h workaround/Windows.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/shobjidl.h workaround/ShObjIdl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/strsafe.h workaround/Strsafe.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/psapi.h workaround/Psapi.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/sddl.h workaround/Sddl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/accctrl.h workaround/AccCtrl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/aclapi.h workaround/Aclapi.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/oleidl.h workaround/OleIdl.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/include/shlobj.h workaround/Shlobj.h)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/lib/libws2_32.a workaround/libWs2_32.a)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/lib/libiphlpapi.a workaround/libIphlpapi.a)
    create_symlink(/usr/${CMAKE_SYSTEM_PROCESSOR}-w64-mingw32/lib/libwldap32.a workaround/libWldap32.a)
    include_directories(${CMAKE_BINARY_DIR}/workaround)
    link_libraries(-L${CMAKE_BINARY_DIR}/workaround)
endif ()

if (URHO3D_SSE AND NOT MSVC)
    # Build engine and it's dependencies with SSE/SSE2 enabled.
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse -msse2")
endif ()

add_subdirectory(Source)

# Print current build configuration
message(STATUS "  Urho3D Configuration:")
if (URHO3D_STATIC_RUNTIME)
    message(STATUS "  Runtime         STATIC")
else ()
    message(STATUS "  Runtime         SHARED")
endif ()
if (BUILD_SHARED_LIBS)
    message(STATUS "  Library         SHARED")
else ()
    message(STATUS "  Library         STATIC")
endif ()
message(STATUS "  SSE             ${URHO3D_SSE}")
message(STATUS "  2D              ${URHO3D_URHO2D}")
message(STATUS "  IK              ${URHO3D_IK}")
message(STATUS "  SystemUI        ${URHO3D_SYSTEMUI}")
message(STATUS "  Logging         ${URHO3D_LOGGING}")
message(STATUS "  Navigation      ${URHO3D_NAVIGATION}")
message(STATUS "  Network         ${URHO3D_NETWORK}")
message(STATUS "  Physics         ${URHO3D_PHYSICS}")
message(STATUS "  Profiling       ${URHO3D_PROFILING}")
message(STATUS "  Samples         ${URHO3D_SAMPLES}")
message(STATUS "  Extras          ${URHO3D_EXTRAS}")
message(STATUS "  WebP            ${URHO3D_WEBP}")
message(STATUS "  Tools           ${URHO3D_TOOLS}")
if (TARGET profiler_gui)
    message(STATUS "     Profiler GUI ${URHO3D_PROFILING}")
endif ()
