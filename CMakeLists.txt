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
cmake_minimum_required (VERSION 3.25)

# Get rid of obscene amount of CMake-added targets we do not use.
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

# Set C++ standard
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set (CMAKE_CXX_STANDARD 17)
endif ()

if (POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif ()

# Set project name and languages
project (rbfx C CXX)
if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif ()

if (BUILD_SHARED_LIBS OR WEB)
    set (CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

# Set CMake modules search path
set (CMAKE_MODULE_PATH ${rbfx_SOURCE_DIR}/CMake/Modules)

# Default install component
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME SDK)

# Generate ThirdParty version hash based on git-tracked files
include(${rbfx_SOURCE_DIR}/CMake/Modules/GetThirdPartyHash.cmake)
get_thirdparty_hash("${rbfx_SOURCE_DIR}/Source/ThirdParty" URHO3D_THIRDPARTY_VERSION HASH_FORMAT version)
get_thirdparty_hash("${rbfx_SOURCE_DIR}/Source/ThirdParty" URHO3D_THIRDPARTY_SHORT_HASH HASH_FORMAT short)
message(STATUS "ThirdParty version: ${URHO3D_THIRDPARTY_VERSION} | ${URHO3D_THIRDPARTY_SHORT_HASH}")

# Try to find ThirdParty as an external package first
# Whether we use CCache depends on result of this call
if (NOT URHO3D_THIRDPARTY_VERSION VERSION_EQUAL "0.0.0")
    find_package(Urho3DThirdParty ${URHO3D_THIRDPARTY_VERSION} EXACT NO_CMAKE_INSTALL_PREFIX)
endif ()

# Disable ccache
if (URHO3D_PRISTINE_FULL_BUILDS OR (ENV{CI} AND NOT Urho3DThirdParty_FOUND))
    set(ENV{CCACHE_DISABLE} 1)
    message(STATUS "CCache disabled")
endif ()

# Generate platform tag
include(PlatformTag)
rbfx_get_platform_tag(RBFX_PLATFORM_TAG TARGET)
message(STATUS "Target platform tag: ${RBFX_PLATFORM_TAG}")

# Include common utilities
include(UrhoCommon)

# Find threading libraries (required by Tracy and other components)
find_package(Threads REQUIRED)

# Enable testing
if (URHO3D_TESTING)
    include (CTest)
    enable_testing ()
endif ()

# Set compiler variable
set ("${CMAKE_CXX_COMPILER_ID}" ON)
if (WEB)
    # Ensure EVERYTHING is compiled with threads support for emscripten.
    add_compile_options(-pthread -sUSE_PTHREADS=1)
else ()
    set (CMAKE_INSTALL_RPATH "$ORIGIN")
    if (GNU OR MINGW)
        if (NOT "$ENV{CI}" STREQUAL "")
            # Disable debugging info completely for CI builds.
            ucm_replace_flag(-g -g0 CONFIG Debug RelWithDebInfo)
        else ()
            # Reduce size of debugging information.
            ucm_replace_flag(-g -g1 CONFIG RelWithDebInfo)
            # Use only debug-friendly optimizations.
            ucm_replace_flag(-O2 -Og CONFIG RelWithDebInfo)
        endif ()
    endif ()
endif ()

# Configure variables
set (URHO3D_URL "https://github.com/urho3d/Urho3D")
set (URHO3D_DESCRIPTION "Urho3D is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE (http://www.ogre3d.org) and Horde3D (http://www.horde3d.org).")
execute_process (COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE URHO3D_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
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

if (WIN32)
    if (NOT DEFINED URHO3D_WINVER)
        if (UWP OR NOT URHO3D_GRAPHICS_NO_D3D12)
            set (URHO3D_WINVER 0x0A00)
        else ()
            set (URHO3D_WINVER 0x0601)
        endif ()
    endif ()
    if (UWP)
        ucm_add_flags(/ZW)
    endif ()
    add_definitions(-DWINVER=${URHO3D_WINVER} -D_WIN32_WINNT=${URHO3D_WINVER} -D_WIN32_WINDOWS=${URHO3D_WINVER})
endif ()

if (UWP)
    # archiving object file compiled with /ZW into a static library; note that when authoring Windows Runtime types it is not recommended to link with a static library that contains Windows Runtime metadata unless /WHOLEARCHIVE is specified to include everything from the static library
    set(CMAKE_STATIC_LINKER_FLAGS /ignore:4264)
endif ()

if (NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND MACOS)
    set (CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
endif ()

if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif ()
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif ()
if (NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
endif ()

if (NOT DEFINED VS_DEBUGGER_WORKING_DIRECTORY)
    set (VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif ()

# Configure for web
if (WEB)
    # Emscripten-specific setup
    if (EMSCRIPTEN_VERSION VERSION_LESS 3.1.24)
        message(FATAL_ERROR "Unsupported compiler version")
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --profiling -Wno-warn-absolute-paths -Wno-unknown-warning-option -Wno-deprecated-builtins")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --profiling -Wno-warn-absolute-paths -Wno-unknown-warning-option -Wno-deprecated-builtins")
    if (URHO3D_THREADING)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_PTHREADS=1")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")
    endif ()
    if (CI_WEB_BUILD)
        set (CMAKE_C_FLAGS_DEBUG "-O0 -DNDEBUG")
        set (CMAKE_CXX_FLAGS_DEBUG "-O0 -DNDEBUG")
        set (CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
        set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    else ()
        set (CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
        set (CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    endif ()
    # Remove variables to make the -O3 regalloc easier, embed data in asm.js to reduce number of moving part
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s AGGRESSIVE_VARIABLE_ELIMINATION=1")
    set (CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -s AGGRESSIVE_VARIABLE_ELIMINATION=1")
endif ()

if (NOT APPLE AND CLANG)
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.1)
        # Workaround for Clang 7.0.1 and above until the Bullet upstream has fixed the Clang 7 diagnostic checks issue (see https://github.com/bulletphysics/bullet3/issues/2114)
        ucm_add_flags(-Wno-argument-outside-range)
    endif ()
endif ()

if (MINGW AND CROSS_TARGET)
    # Workarounds for crosscompiling MinGW on linux. EASTL uses uppser-case headers where MinGW has them lower-case.
    foreach (HEADER Windows.h WinSock2.h ShellAPI.h XInput.h Rpc.h Shlwapi.h)
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
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (URHO3D_SSE AND NOT MSVC)
    # Build engine and it's dependencies with SSE/SSE2 enabled.
    ucm_add_flags(-msse -msse2)
endif ()

if (NOT "$ENV{CI}" STREQUAL "")
    if (MSVC)
        ucm_replace_flag("/W[0-3]" "/W0" REGEX)
    else ()
        ucm_add_flags("-w")
    endif ()
elseif (MSVC)
    ucm_replace_flag("/W[0-3]" "/W1" REGEX)
endif ()

# Temporary: Workaround for MSVC crash.
if (MSVC)
    add_compile_definitions(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
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

rbfx_configure_cmake_props()

# Print current build configuration
message(STATUS "Engine Features:")
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
message(STATUS "  2D              ${URHO3D_URHO2D}")
message(STATUS "  Compute         ${URHO3D_COMPUTE}")
message(STATUS "  Glow            ${URHO3D_GLOW}")
message(STATUS "  IK              ${URHO3D_IK}")
message(STATUS "  Navigation      ${URHO3D_NAVIGATION}")
message(STATUS "  Network         ${URHO3D_NETWORK}")
message(STATUS "  Particle Graph  ${URHO3D_PARTICLE_GRAPH}")
message(STATUS "  Physics         ${URHO3D_PHYSICS}")
message(STATUS "  Physics2D       ${URHO3D_PHYSICS2D}")
message(STATUS "  Plugins         ${URHO3D_PLUGINS}")
message(STATUS "  RmlUI           ${URHO3D_RMLUI}")
message(STATUS "  SPIR-V Cross    ${URHO3D_SHADER_TRANSLATOR}")
message(STATUS "  SPIR-V Optimize ${URHO3D_SHADER_OPTIMIZER}")
message(STATUS "  Threading       ${URHO3D_THREADING}")
message(STATUS "  WebP            ${URHO3D_WEBP}")
message(STATUS "  XR              ${URHO3D_XR}")
message(STATUS "  CSharp          ${URHO3D_CSHARP}")
if (URHO3D_CSHARP)
    message(STATUS "    Targeting:    ${URHO3D_NETFX}")
    message(STATUS "    Runtime:      ${URHO3D_NETFX_RUNTIME} (${URHO3D_NETFX_RUNTIME_IDENTIFIER})")
endif ()
message(STATUS "Engine Components:")
if (URHO3D_NO_EDITOR_PLAYER_EXE)
    message(STATUS "  Player          ${URHO3D_PLAYER} (manual)")
    message(STATUS "  Editor          ${URHO3D_EDITOR} (manual)")
else ()
    message(STATUS "  Player          ${URHO3D_PLAYER}")
    message(STATUS "  Editor          ${URHO3D_EDITOR}")
endif ()
message(STATUS "  Samples         ${URHO3D_SAMPLES}")
message(STATUS "  Testing         ${URHO3D_TESTING}")
message(STATUS "  Tools           ${URHO3D_TOOLS}")
message(STATUS "  Extras          ${URHO3D_EXTRAS}")
message(STATUS "Engine Tweaks:")
if (WIN32)
    message(STATUS "  MiniDumps       ${URHO3D_MINIDUMPS}")
endif()
message(STATUS "  Debug Graphics  ${URHO3D_DEBUG_GRAPHICS}")
message(STATUS "  Hash Debugging  ${URHO3D_HASH_DEBUG}")
message(STATUS "  Logging         ${URHO3D_LOGGING}")
message(STATUS "  Packaging       ${URHO3D_PACKAGING}")
message(STATUS "  Profiling       ${URHO3D_PROFILING}")
message(STATUS "  SSE             ${URHO3D_SSE}")
message(STATUS "  SystemUI        ${URHO3D_SYSTEMUI}")
message(STATUS "  Urho3DAll.h     ${URHO3D_MONOLITHIC_HEADER}")
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

################################################### CPack Configuration ################################################

# Set up basic CPack configuration
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION "${URHO3D_VERSION}")

# Include CPack module
include(CPack)

# Create packaging targets for each component
if (NOT CMAKE_SKIP_INSTALL_RULES)
    create_pack_target("SDK")
    create_pack_target("Samples")
    create_pack_target("Tools")
    create_pack_all_target()
endif ()

################################################### SDK preparation ####################################################

# Install CMake modules and toolchains provided by and for Urho3D
install (DIRECTORY ${rbfx_SOURCE_DIR}/CMake/Toolchains/ DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/CMake/Toolchains)    # Note: the trailing slash is significant
install (DIRECTORY ${rbfx_SOURCE_DIR}/CMake/Modules/ DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/CMake/Modules/)
# Install CMake build scripts
file (GLOB CMAKE_SCRIPTS ${rbfx_SOURCE_DIR}/Script/*${SCRIPT_EXT})
install (PROGRAMS ${CMAKE_SCRIPTS} DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/Scripts)
# Install data files
file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if (NOT ANDROID)
    # Multi-config projects store data files in parent dir of binary files.
    string (REPLACE "/${URHO3D_CONFIG}" "" RUNTIME_OUTPUT_DIRECTORY_NO_CONFIG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    # Copy data directories for development
    set (INSTALL_DATA_DIRS Data CoreData EditorData Autoload)
    foreach(dir ${INSTALL_DATA_DIRS})
        if (URHO3D_COPY_DATA_DIRS)
            file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${dir} DESTINATION ${RUNTIME_OUTPUT_DIRECTORY_NO_CONFIG}/${dir}/../ FOLLOW_SYMLINK_CHAIN)
        else ()
            create_symlink(${CMAKE_CURRENT_SOURCE_DIR}/bin/${dir} ${RUNTIME_OUTPUT_DIRECTORY_NO_CONFIG}/${dir})
        endif ()
    endforeach()

    # Install data directories with appropriate components
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/CoreData DESTINATION ${CMAKE_INSTALL_BINDIR_BASE} COMPONENT SDK)
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/CoreData DESTINATION ${CMAKE_INSTALL_BINDIR_BASE} COMPONENT Tools)
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/CoreData DESTINATION ${CMAKE_INSTALL_BINDIR_BASE} COMPONENT Samples)

    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/EditorData DESTINATION ${CMAKE_INSTALL_BINDIR_BASE} COMPONENT SDK)
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/EditorData DESTINATION ${CMAKE_INSTALL_BINDIR_BASE} COMPONENT Tools)

    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/Data DESTINATION ${CMAKE_INSTALL_BINDIR_BASE} COMPONENT Samples)
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/Autoload DESTINATION ${CMAKE_INSTALL_BINDIR_BASE} COMPONENT Samples)
endif ()
install (FILES ${rbfx_SOURCE_DIR}/bin/shell.html DESTINATION ${CMAKE_INSTALL_BINDIR_BASE})
