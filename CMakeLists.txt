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

# Set CMake minimum version. Also set in android/build.gradle for android projects.
cmake_minimum_required (VERSION 3.14)

# https://cmake.org/cmake/help/v3.18/policy/CMP0077.html
# Note that cmake_minimum_required() + project() resets policies, so dependencies using lower CMake version would not
# properly accept options before we add_subdirectory() them without setting this policy to NEW __in their build script__.
cmake_policy(SET CMP0077 NEW)

# Set C++ standard
set (CMAKE_CXX_STANDARD 17)

# Set project name
project (rbfx)

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

if (WEB AND "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    # Workaround for following error:
    #   The install of the Samples target requires changing an RPATH from the build
    #   tree, but this is not supported with the Ninja generator unless on an
    #   ELF-based platform.  The CMAKE_BUILD_WITH_INSTALL_RPATH variable may be set
    #   to avoid this relinking step.
    set (CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif ()

# Include file that sets up ccache support for Xcode.
include(CCacheXcode)

# Include file that sets up all configurable properties
include(UrhoOptions)

# Include common utilitles
include(UrhoCommon)

# Enable common build options
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set (CMAKE_CXX_STANDARD 17)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CLANG ON)
    set (GNU ON)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (GCC ON)
    set (GNU ON)
endif()

# Set compiler variable
set ("${CMAKE_CXX_COMPILER_ID}" ON)
if (NOT WEB)
    set (CMAKE_INSTALL_RPATH "$ORIGIN")
    if (GNU OR MINGW)
        # Reduce size of debugging information in RelWithDebInfo builds.
        ucm_replace_flag(-g -g1 CONFIG RelWithDebInfo)
        ucm_replace_flag(-O2 -Og CONFIG RelWithDebInfo)
    endif ()
endif ()

# Configure variables
set (URHO3D_URL "https://github.com/urho3d/Urho3D")
set (URHO3D_DESCRIPTION "Urho3D is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE (http://www.ogre3d.org) and Horde3D (http://www.horde3d.org).")
execute_process (COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/GetUrhoRevision.cmake WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE URHO3D_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
string (REGEX MATCH "([^.]+)\\.([^.]+)\\.(.+)" MATCHED "${URHO3D_VERSION}")

# Setup SDK install destinations
if (WIN32)
    set (SCRIPT_EXT .bat)
else ()
    set (SCRIPT_EXT .sh)
endif ()
if (ANDROID)
    # For Android platform, install to a path based on the chosen Android ABI, e.g. libs/armeabi-v7a
    set (LIB_SUFFIX s/${ANDROID_NDK_ABI_NAME})
endif ()
set (DEST_BASE_INCLUDE_DIR include)
set (DEST_INCLUDE_DIR ${DEST_BASE_INCLUDE_DIR}/Urho3D)
set (DEST_BIN_DIR bin)
set (DEST_SHARE_DIR share)
set (DEST_RESOURCE_DIR ${DEST_BIN_DIR})
set (DEST_BUNDLE_DIR ${DEST_SHARE_DIR}/Applications)
set (DEST_ARCHIVE_DIR lib)
set (DEST_PKGCONFIG_DIR ${DEST_ARCHIVE_DIR}/pkgconfig)
set (DEST_THIRDPARTY_HEADERS_DIR ${DEST_INCLUDE_DIR}/ThirdParty)
if (ANDROID)
    set (DEST_LIBRARY_DIR ${DEST_ARCHIVE_DIR})
else ()
    set (DEST_LIBRARY_DIR bin)
endif ()

if (MSVC OR "${CMAKE_GENERATOR}" STREQUAL "Xcode")
    set (MULTI_CONFIG_PROJECT ON)
endif ()
if (MULTI_CONFIG_PROJECT)
    set (DEST_BIN_DIR_CONFIG ${DEST_BIN_DIR}/$<CONFIG>)
    set (DEST_LIBRARY_DIR_CONFIG ${DEST_LIBRARY_DIR}/$<CONFIG>)
    set (DEST_ARCHIVE_DIR_CONFIG ${DEST_ARCHIVE_DIR}/$<CONFIG>)
else ()
    set (DEST_BIN_DIR_CONFIG ${DEST_BIN_DIR})
    set (DEST_LIBRARY_DIR_CONFIG ${DEST_LIBRARY_DIR})
    set (DEST_ARCHIVE_DIR_CONFIG ${DEST_ARCHIVE_DIR})
endif ()
if (WIN32)
    set (WINVER 0x0601)
    add_definitions(-DWINVER=${WINVER} -D_WIN32_WINNT=${WINVER} -D_WIN32_WINDOWS=${WINVER})
endif ()
if (NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND MACOS)
    set (CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
endif ()

if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR})
endif ()
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR})
endif ()
if (NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_ARCHIVE_DIR})
endif ()

if (NOT DEFINED VS_DEBUGGER_WORKING_DIRECTORY)
    set (VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif ()

if (NOT DEFINED URHO3D_64BIT)
    if (CMAKE_SIZEOF_VOID_P MATCHES 8)
        set(URHO3D_64BIT ON)
        set(URHO3D_PLATFORM x64)
    else ()
        set(URHO3D_64BIT OFF)
        set(URHO3D_PLATFORM x86)
    endif ()
endif ()

if (MINGW)
    find_file(DLL_FILE_PATH_1 "libstdc++-6.dll")
    find_file(DLL_FILE_PATH_2 "libgcc_s_seh-1.dll")
    find_file(DLL_FILE_PATH_3 "libwinpthread-1.dll")
    foreach (DLL_FILE_PATH ${DLL_FILE_PATH_1} ${DLL_FILE_PATH_2} ${DLL_FILE_PATH_3})
        if (DLL_FILE_PATH)
            # Copies dlls to bin or tools.
            file (COPY ${DLL_FILE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG})
            if (NOT URHO3D_STATIC_RUNTIME)
                file (COPY ${DLL_FILE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG})
            endif ()
        endif ()
    endforeach ()
endif ()

# Configure for web
if (WEB)
    # Emscripten-specific setup
    if (EMSCRIPTEN_EMCC_VERSION VERSION_LESS 1.31.3)
        message(FATAL_ERROR "Unsupported compiler version")
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
    if (URHO3D_THREADING)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_PTHREADS=1")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")
    endif ()
    set (CMAKE_C_FLAGS_RELEASE "-Oz -DNDEBUG")
    set (CMAKE_CXX_FLAGS_RELEASE "-Oz -DNDEBUG")
    # Remove variables to make the -O3 regalloc easier, embed data in asm.js to reduce number of moving part
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1 --memory-init-file 0")
    set (CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1 --memory-init-file 0")
    # Preserve LLVM debug information, show line number debug comments, and generate source maps; always disable exception handling codegen
    set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
    set (CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
endif ()

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    set (HOST_LINUX 1)
elseif ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set (HOST_WIN32 1)
elseif ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Darwin")
    set (HOST_MACOS 1)
endif ()

if (NOT APPLE AND CLANG)
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.1)
        # Workaround for Clang 7.0.1 and above until the Bullet upstream has fixed the Clang 7 diagnostic checks issue (see https://github.com/bulletphysics/bullet3/issues/2114)
        ucm_add_flags(-Wno-argument-outside-range)
    endif ()
endif ()

if (URHO3D_SSE)
    if (DESKTOP)
        string (TOLOWER "${URHO3D_SSE}" URHO3D_SSE)
        if (MSVC)
            list (APPEND VALID_SSE_OPTIONS sse sse2 avx avx2)
        else ()
            list (APPEND VALID_SSE_OPTIONS sse sse2 sse3 sse4 sse4a sse4.1 sse4.2 avx avx2)
        endif ()
        list (FIND VALID_SSE_OPTIONS "${URHO3D_SSE}" SSE_INDEX)
        if (SSE_INDEX EQUAL -1)
            set (URHO3D_SSE sse2)
        endif ()
        if (MSVC)
            string (TOUPPER "${URHO3D_SSE}" URHO3D_SSE)
        endif ()
    else ()
        set (URHO3D_SSE OFF)
    endif ()
endif ()

if (MINGW AND CROSS_TARGET)
    # Workarounds for crosscompiling MinGW on linux. EASTL uses uppser-case headers where MinGW has them lower-case.
    foreach (HEADER Windows.h WinSock2.h ShellAPI.h XInput.h Rpc.h)
        find_path(${HEADER}_PATH ${HEADER})
        if (NOT ${${HEADER}_PATH})
            string(TOLOWER "${HEADER}" HEADER_LOWER)
            find_path(${HEADER}_PATH ${HEADER_LOWER})
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes)
            create_symlink(${${HEADER}_PATH}/${HEADER_LOWER} ${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes/${HEADER})
        endif ()
    endforeach()
    include_directories(${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes)
endif ()

if (URHO3D_CSHARP)
    if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
        # Workaround for some cases where csc has issues when invoked by CMake.
        set (TERM_WORKAROUND env TERM=xterm)
    elseif ("${CMAKE_GENERATOR}" STREQUAL "Xcode")
        # Xcode adds something to environment which makes msbuild targets fail. Invoke msbuild with clean environment instead.
        set (TERM_WORKAROUND env -i)
    endif ()
    if (CMAKE_VS_MSBUILD_COMMAND)
        set (MSBUILD ${CMAKE_VS_MSBUILD_COMMAND} CACHE STRING "")
    else ()
        find_program(MSBUILD msbuild HINTS $<MACOS:/Library/Frameworks/Mono.framework/Versions/Current/bin> $ENV{PATH})
    endif ()

    if (NOT MSBUILD)
        message(FATAL_ERROR "msbuild executable was not found.")
    endif ()

    if (URHO3D_NETFX MATCHES "net(46|standard2).*")
        set (URHO3D_NETFX_LEGACY_VERSION ON)
    endif ()

    if (WIN32)
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER win7-${URHO3D_PLATFORM})
    elseif (MACOS)
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER osx-${URHO3D_PLATFORM})
    else ()
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER linux-${URHO3D_PLATFORM})
    endif ()
endif ()

if (URHO3D_CSHARP)
    # For .csproj embedded into visual studio solution
    configure_file("${rbfx_SOURCE_DIR}/cmake/CMake.props.in" "${CMAKE_BINARY_DIR}/CMake.props" @ONLY)
    # For .csproj that gets built by cmake invoking msbuild
    set (ENV{CMAKE_GENERATOR} "${CMAKE_GENERATOR}")
    set (ENV{CMAKE_BINARY_DIR "${CMAKE_BINARY_DIR}/")
    set (ENV{RBFX_BINARY_DIR "${rbfx_BINARY_DIR}/")
    set (ENV{CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")

    set (DOTNET_FRAMEWORK_TYPES net46 net461 net462 net47 net471 net472 net48)
    set (DOTNET_FRAMEWORK_VERSIONS v4.6 v4.6.1 v4.6.2 v4.7 v4.7.1 v4.7.2 v4.8)
    list (FIND DOTNET_FRAMEWORK_TYPES ${URHO3D_NETFX} DOTNET_FRAMEWORK_INDEX)
    if (DOTNET_FRAMEWORK_INDEX GREATER -1)
        list (GET DOTNET_FRAMEWORK_VERSIONS ${DOTNET_FRAMEWORK_INDEX} CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION)
    endif ()
    unset (DOTNET_FRAMEWORK_INDEX)

    if (NOT SWIG_LIB)
        set (SWIG_DIR ${rbfx_SOURCE_DIR}/Source/ThirdParty/swig/Lib)
    endif ()
    include(UrhoSWIG)
endif()

# Workarounds. They are mirrored in Urho3D target build script so that projects linking to it would inherit settings.
if (WEB OR ANDROID)
    set (URHO3D_CXX_STANDARD ${CMAKE_CXX_STANDARD})
    ucm_add_flags(CXX -std=gnu++${CMAKE_CXX_STANDARD})
    # NDK bundles old cmake and insists on usiing it, That version chokes on standard being set to 17 while
    # crosscompiler works just fine.
    unset (CMAKE_CXX_STANDARD)
endif ()
if (CLANG)
    if (APPLE)
        # Ensure that everything built by this project use these libraries. Urho3D target also exports them so all downstream
        # projects linking to the engine get them as well.
        ucm_add_flags(C CXX -stdlib=libc++)
        link_libraries(c++ c++abi)
    elseif (DESKTOP AND UNIX)
        # Android ndk handles linking runtime library. Emscripten does not need these.
        link_libraries(stdc++ m)
    endif ()
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

if (DESKTOP)
    # Mobile/web toolchains manage runtime libraries themselves, we should not interfere.
    if (URHO3D_STATIC_RUNTIME)
        ucm_set_runtime(STATIC)
    else ()
        ucm_set_runtime(DYNAMIC)
    endif ()
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
message(STATUS "  Graphics        ${URHO3D_GRAPHICS_API}")
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
message(STATUS "  RmlUI           ${URHO3D_RMLUI}")
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
install (DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/Toolchains/ DESTINATION ${DEST_SHARE_DIR}/cmake/Toolchains)    # Note: the trailing slash is significant
install (DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/Modules/ DESTINATION ${DEST_SHARE_DIR}/cmake/Modules/)
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
