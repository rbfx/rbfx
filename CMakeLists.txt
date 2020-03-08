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
if (ANDROID)
    cmake_minimum_required (VERSION 3.6)
else ()
    cmake_minimum_required (VERSION 3.14)
endif ()

# Set C++ standard
set (CMAKE_CXX_STANDARD 17)

# Set project name
project (rbfx)

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules)

if (WEB AND "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    # Workaround for following error:
    #   The install of the Samples target requires changing an RPATH from the build
    #   tree, but this is not supported with the Ninja generator unless on an
    #   ELF-based platform.  The CMAKE_BUILD_WITH_INSTALL_RPATH variable may be set
    #   to avoid this relinking step.
    set (CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif ()

# Include file that sets up all configurable properties
include(UrhoOptions)

# Include common utilitles
include(UrhoCommon)

# Enable common build options
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set (CMAKE_CXX_STANDARD 17)
endif ()

# Workarounds. They are mirrored in Urho3D target build script so that projects linking to it would inherit settings.
if (WEB OR ANDROID)
    set (URHO3D_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    ucm_add_flags(CXX -std=gnu++${CMAKE_CXX_STANDARD})
    # NDK bundles old cmake and insists on usiing it, That version chokes on standard being set to 17 while
    # crosscompiler works just fine.
    unset (CMAKE_CXX_STANDARD)
endif ()
if (APPLE)
    ucm_add_flags(CXX -stdlib=libc++)
endif ()

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (BUILD_SHARED_LIBS)
    set (CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (URHO3D_SSE AND NOT MSVC)
    # Build engine and it's dependencies with SSE/SSE2 enabled.
    ucm_add_flags(-msse -msse2)
endif ()

if (NOT "$ENV{CI}" STREQUAL "")
	if (MSVC)
		ucm_add_flags("/W0")
	else ()
		ucm_add_flags("-w")
	endif ()
elseif (MSVC)
	ucm_add_flags("/W1")
endif ()

if (URHO3D_STATIC_RUNTIME)
    ucm_set_runtime(STATIC)
else ()
    ucm_set_runtime(DYNAMIC)
endif ()

add_subdirectory(Source)

# Print current build configuration
message(STATUS "Urho3D Configuration:")
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
message(STATUS "  Glow            ${URHO3D_GLOW}")
message(STATUS "  IK              ${URHO3D_IK}")
message(STATUS "  Threading       ${URHO3D_THREADING}")
message(STATUS "  Navigation      ${URHO3D_NAVIGATION}")
message(STATUS "  Network         ${URHO3D_NETWORK}")
message(STATUS "  Physics         ${URHO3D_PHYSICS}")
message(STATUS "  Samples         ${URHO3D_SAMPLES}")
message(STATUS "  WebP            ${URHO3D_WEBP}")
message(STATUS "  CSharp          ${URHO3D_CSHARP}")
if (WIN32)
    message(STATUS "  MiniDumps       ${URHO3D_MINIDUMPS}")
endif()
message(STATUS "Developer options:")
message(STATUS "  SystemUI        ${URHO3D_SYSTEMUI}")
message(STATUS "  Logging         ${URHO3D_LOGGING}")
message(STATUS "  Profiling       ${URHO3D_PROFILING}")
message(STATUS "  Extras          ${URHO3D_EXTRAS}")
message(STATUS "  Tools           ${URHO3D_TOOLS}")
message(STATUS "  Docs            ${URHO3D_DOCS}")
message(STATUS "  Compute         ${URHO3D_COMPUTE}")
if (TARGET Profiler)
    message(STATUS "     Profiler GUI ${URHO3D_PROFILING}")
endif ()

# clang-tidy target
find_program(CLANG_TIDY clang-tidy)
if (CLANG_TIDY)
    file (GLOB_RECURSE SOURCE_FILES
        Source/Samples/**.cpp Source/Samples/**.h Source/Samples/**.hpp Source/Samples/**.inl
        Source/Urho3D/**.cpp Source/Urho3D/**.h Source/Urho3D/**.hpp
        Source/Tools/**.cpp Source/Tools/**.h Source/Tools/**.hpp
    )
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY} -p . -export-fixes=clang-tidy.yml -fix ${SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_target_properties(tidy PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
endif()

################################################### SDK preparation ####################################################
if (MINI_URHO)
    return ()
endif ()

if (URHO3D_DOCS)
    add_subdirectory(Docs)
endif ()

# Install CMake modules and toolchains provided by and for Urho3D
install (DIRECTORY ${CMAKE_SOURCE_DIR}/CMake/Toolchains/ DESTINATION ${DEST_SHARE_DIR}/CMake/Toolchains)    # Note: the trailing slash is significant
install (DIRECTORY ${CMAKE_SOURCE_DIR}/CMake/Modules/ DESTINATION ${DEST_SHARE_DIR}/CMake/Modules/)
# Install CMake build scripts
file (GLOB CMAKE_SCRIPTS ${CMAKE_SOURCE_DIR}/Script/*${SCRIPT_EXT})
install (PROGRAMS ${CMAKE_SCRIPTS} DESTINATION ${DEST_SHARE_DIR}/Scripts)
# Install data files
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if (NOT ANDROID)
    file (MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Autoload/)
    foreach(dir Data CoreData EditorData Autoload/LargeData)
        create_symlink(${CMAKE_CURRENT_SOURCE_DIR}/bin/${dir} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${dir})
        install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${dir} DESTINATION ${DEST_RESOURCE_DIR}/${dir}/../)
    endforeach()
endif ()

set (URHO3D_CMAKE_CONFIGURED ON CACHE STRING "" FORCE)
