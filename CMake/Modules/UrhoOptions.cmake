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

# Source environment
if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    execute_process(COMMAND cmd /c set OUTPUT_VARIABLE ENVIRONMENT)
else ()
    execute_process(COMMAND env OUTPUT_VARIABLE ENVIRONMENT)
endif ()
string(REGEX REPLACE "=[^\n]*\n?" ";" ENVIRONMENT "${ENVIRONMENT}")
set(IMPORT_URHO3D_VARIABLES_FROM_ENV BUILD_SHARED_LIBS SWIG_EXECUTABLE SWIG_DIR)
foreach(key ${ENVIRONMENT})
    list (FIND IMPORT_URHO3D_VARIABLES_FROM_ENV ${key} _index)
    if ("${key}" MATCHES "^(URHO3D_|CMAKE_|ANDROID_).+" OR ${_index} GREATER -1)
        if (NOT DEFINED ${key})
            set (${key} $ENV{${key}} CACHE STRING "" FORCE)
        endif ()
    endif ()
endforeach()

# https://cmake.org/cmake/help/v3.18/policy/CMP0077.html
# Note that cmake_minimum_required() + project() resets policies, so dependencies using lower CMake version would not
# properly accept options before we add_subdirectory() them without setting this policy to NEW __in their build script__.
if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
endif ()

foreach (feature ${URHO3D_FEATURES})
    set (URHO3D_${feature} ON)
endforeach()

include(CMakeDependentOption)

# Set MULTI_CONFIG_PROJECT if applicable
if (MSVC OR "${CMAKE_GENERATOR}" STREQUAL "Xcode")
    set (MULTI_CONFIG_PROJECT ON)
endif ()

# Set platform and compiler variables
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set (LINUX ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set (UWP ON)
elseif (APPLE AND NOT IOS)
    set (MACOS ON)
endif ()

if (ANDROID OR IOS)
    set (MOBILE ON)
endif ()

if (APPLE OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set (APPLE ON)
endif ()

# Even though UWP can run on desktop, we do not treat it as a desktop platform, because it behaves more like a mobile app.
if ((WIN32 OR LINUX OR MACOS) AND NOT EMSCRIPTEN AND NOT MOBILE AND NOT UWP)
    set (DESKTOP ON)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set (CLANG ON)
    set (GNU ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (GCC ON)
    set (GNU ON)
endif()

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set (HOST_LINUX ON)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set (HOST_WIN32 ON)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set (HOST_MACOS ON)
endif ()

# Determine build arch
string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" CMAKE_GENERATOR_PLATFORM)
set(URHO3D_PLATFORM "${CMAKE_GENERATOR_PLATFORM}")
if (CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(URHO3D_64BIT ON)
else ()
    set(URHO3D_64BIT OFF)
endif ()

# TODO: Arm support.
# NOTE: URHO3D_PLATFORM is only used in .csproj
if (NOT URHO3D_PLATFORM)
    if (URHO3D_64BIT)
        set (URHO3D_PLATFORM x64)
    else ()
        set (URHO3D_PLATFORM x86)
    endif ()
endif ()
if (URHO3D_PLATFORM STREQUAL "win32")
    set (URHO3D_PLATFORM x86)
endif ()

# Build properties
option(BUILD_SHARED_LIBS                        "Build engine as shared library."       ON)
option(URHO3D_ENABLE_ALL                        "Enable (almost) all engine features."  ON)
option(URHO3D_STATIC_RUNTIME                    "Link to static runtime"               OFF)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.16)
    option(URHO3D_PCH                           "Enable precompiled header"                              ON)
else ()
    set (URHO3D_PCH OFF)
endif ()

if (UWP)
    set (URHO3D_SSE OFF)
else ()
    set (URHO3D_SSE           SSE2 CACHE STRING "Enable SSE instructions")
endif ()

# Determine SSE support.
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

# Our version of cmake_dependent_option, but with a twist. When "FORCE_OFF" is not specified, option value is set to
# "VALUE" or "VALUE_OFF" depending on whether "DEPENDS" evaluates to true or false, unless user set a custom value. If
# custom value was set and "DEPENDS" changed - "OPTION" value will not be modified.
function (rbfx_dependent_option OPTION DOCSTRING VALUE DEPENDS VALUE_OFF)
    cmake_parse_arguments(ARG "FORCE_OFF" "" "" ${ARGN})
    if (ARG_FORCE_OFF)
        cmake_dependent_option(${OPTION} "${DOCSTRING}" ${VALUE} "${${DEPENDS}}" ${VALUE_OFF})
    else ()
        if (${DEPENDS})
            set (set_value ${VALUE})
        else ()
            set (set_value ${VALUE_OFF})
        endif ()
        if (${DEPENDS} AND ("${${OPTION}}" STREQUAL "${VALUE_OFF}"))
            set (force FORCE)
            set (set_value ${VALUE})
        elseif (NOT ${DEPENDS} AND ("${${OPTION}}" STREQUAL "${VALUE}"))
            set (VALUE "${VALUE_OFF}")
            set (force FORCE)
        endif()
        set (${OPTION} "${set_value}" CACHE STRING ${DOCSTRING} ${force})
    endif()
endfunction ()

# Subsystems
cmake_dependent_option(URHO3D_GLOW               "Lightmapping subsystem enabled"                        ${URHO3D_ENABLE_ALL} "DESKTOP;NOT MINGW"             OFF)
option                (URHO3D_IK                 "Inverse kinematics subsystem enabled"                  ${URHO3D_ENABLE_ALL})
option                (URHO3D_LOGGING            "Enable logging subsystem"                              ${URHO3D_ENABLE_ALL})
option                (URHO3D_NAVIGATION         "Navigation subsystem enabled"                          ${URHO3D_ENABLE_ALL})
option                (URHO3D_NETWORK            "Networking subsystem enabled"                          ${URHO3D_ENABLE_ALL})
option                (URHO3D_PHYSICS            "Physics subsystem enabled"                             ${URHO3D_ENABLE_ALL})
cmake_dependent_option(URHO3D_PROFILING          "Profiler support enabled"                              ${URHO3D_ENABLE_ALL} "NOT EMSCRIPTEN;NOT MINGW;NOT UWP"     OFF)
cmake_dependent_option(URHO3D_PROFILING_FALLBACK "Profiler uses low-precision timer"                     OFF                  "URHO3D_PROFILING"              OFF)
cmake_dependent_option(URHO3D_PROFILING_SYSTRACE "Profiler systrace support enabled"                     OFF                  "URHO3D_PROFILING"              OFF)
option                (URHO3D_SYSTEMUI           "Build SystemUI subsystem"                              ${URHO3D_ENABLE_ALL})
option                (URHO3D_URHO2D             "2D subsystem enabled"                                  ${URHO3D_ENABLE_ALL})
option                (URHO3D_PHYSICS2D          "2D physics subsystem enabled"                          ${URHO3D_ENABLE_ALL})
option                (URHO3D_RMLUI              "HTML subset UIs via RmlUI middleware"                  ${URHO3D_ENABLE_ALL})
option                (URHO3D_PARTICLE_GRAPH     "Particle Graph Effects"                                ${URHO3D_ENABLE_ALL})
option                (URHO3D_ACTIONS            "Tweening actions"                                      ${URHO3D_ENABLE_ALL})
option                (URHO3D_SHADER_TRANSLATOR  "Enable shader translation from universal GLSL shaders to other GAPI via glslang and SPIRV-Cross" ${URHO3D_ENABLE_ALL})
option                (URHO3D_SHADER_OPTIMIZER   "Enable shader optimization via SPIRV-Tools"            ${URHO3D_ENABLE_ALL})

# User should extend AndroidManifest.xml and attach libopenxr_loader.so to the application.
# See https://developer.oculus.com/documentation/native/android/mobile-openxr/ for details.
cmake_dependent_option(URHO3D_OCULUS_QUEST       "Enable experimental native build for Oculus Quest. See notes in UrhoOptions.cmake!" OFF "ANDROID" OFF)
# TODO: Extend platform support.
cmake_dependent_option(URHO3D_XR                 "Enable OpenXR support"                                 ${URHO3D_ENABLE_ALL} "WIN32 OR URHO3D_OCULUS_QUEST;NOT MINGW;NOT UWP" OFF)

# Features
cmake_dependent_option(URHO3D_CSHARP             "Enable C# support"                                     OFF                  "BUILD_SHARED_LIBS;NOT MINGW"   OFF)
# Valid values at https://docs.microsoft.com/en-us/dotnet/standard/frameworks
# At the moment only netstandard2.1 supported
set(URHO3D_NETFX netstandard2.1 CACHE STRING "TargetFramework value for .NET libraries")
set_property(CACHE URHO3D_NETFX PROPERTY STRINGS netstandard2.1)
set(URHO3D_NETFX_RUNTIME_VERSION OFF CACHE STRING "Version of runtime to use.")
option                (URHO3D_DEBUG_ASSERT       "Enable Urho3D assert macros"                           ${URHO3D_ENABLE_ALL}                                    )
cmake_dependent_option(URHO3D_FILEWATCHER        "Watch filesystem for resource changes"                 ${URHO3D_ENABLE_ALL} "URHO3D_THREADING;NOT UWP"      OFF)
option                (URHO3D_HASH_DEBUG         "Enable StringHash name debugging"                      ${URHO3D_ENABLE_ALL}                                    )
option                (URHO3D_MONOLITHIC_HEADER  "Create Urho3DAll.h which includes all engine headers." OFF                                                     )
cmake_dependent_option(URHO3D_MINIDUMPS          "Enable writing minidumps on crash"                     ${URHO3D_ENABLE_ALL} "MSVC;NOT UWP"                  OFF)
cmake_dependent_option(URHO3D_PLUGINS            "Enable plugins"                                        ${URHO3D_ENABLE_ALL} "NOT EMSCRIPTEN;NOT UWP"               OFF)
cmake_dependent_option(URHO3D_THREADING          "Enable multithreading"                                 ${URHO3D_ENABLE_ALL} "NOT EMSCRIPTEN"                       OFF)
option                (URHO3D_WEBP               "WebP support enabled"                                  ${URHO3D_ENABLE_ALL}                                    )
cmake_dependent_option(URHO3D_TESTING            "Enable unit tests"                                     OFF                  "NOT EMSCRIPTEN;NOT MOBILE;NOT UWP"    OFF)
option                (URHO3D_PACKAGING          "Enable *.pak file creation"                            OFF                                                     )
# Web
cmake_dependent_option(EMSCRIPTEN_WASM           "Use wasm instead of asm.js"                            ON                   "EMSCRIPTEN"                           OFF)
set(EMSCRIPTEN_TOTAL_MEMORY 128 CACHE STRING  "Memory limit in megabytes. Set to 0 for dynamic growth. Must be multiple of 64KB.")

# Graphics configuration
option                (URHO3D_DEBUG_GRAPHICS     "Enable debug checks in renderer"                       OFF)
option                (URHO3D_GRAPHICS_NO_GL     "Disable OpenGL backend in renderer"                    OFF)
cmake_dependent_option(URHO3D_GRAPHICS_NO_D3D11  "Disable Direct3D11 backend in renderer"                OFF "URHO3D_SHADER_TRANSLATOR" ON)
cmake_dependent_option(URHO3D_GRAPHICS_NO_D3D12  "Disable Direct3D12 backend in renderer"                OFF "URHO3D_SHADER_TRANSLATOR" ON)
cmake_dependent_option(URHO3D_GRAPHICS_NO_VULKAN "Disable Vulkan backend in renderer"                    OFF "URHO3D_SHADER_TRANSLATOR" ON)

# Misc
rbfx_dependent_option(URHO3D_PLUGIN_LIST "List of plugins to be statically linked with Editor and Player executables" "Sample.103_GamePlugin;Sample.113_InputLogger" URHO3D_SAMPLES "")
option               (URHO3D_PARALLEL_BUILD     "MSVC-only: enable parallel builds. A bool or a number of processors to use." ON)

option(URHO3D_PLAYER                            "Build player application"                              ${URHO3D_ENABLE_ALL})
cmake_dependent_option(URHO3D_EDITOR            "Build editor application"                              ${URHO3D_ENABLE_ALL} "DESKTOP"                       OFF)
cmake_dependent_option(URHO3D_EXTRAS            "Build extra tools"                                     ${URHO3D_ENABLE_ALL} "NOT EMSCRIPTEN;NOT MOBILE;NOT UWP"    OFF)
cmake_dependent_option(URHO3D_TOOLS             "Tools enabled. Bool or a list of tool target names."   ${URHO3D_ENABLE_ALL} "DESKTOP"                       OFF)
option(URHO3D_SAMPLES                           "Build samples"                                         OFF)
cmake_dependent_option(URHO3D_MERGE_STATIC_LIBS "Merge third party dependency libs to Urho3D.a"         OFF "NOT BUILD_SHARED_LIBS"                          OFF)
option(URHO3D_NO_EDITOR_PLAYER_EXE              "Do not build editor or player executables."            OFF)
option(URHO3D_COPY_DATA_DIRS                    "Copy data dirs instead of sym link."                   OFF)

if (WIN32)
    option(URHO3D_WIN32_CONSOLE "Show log messages in win32 console"                     OFF)
endif ()

if (URHO3D_CSHARP)
    set (URHO3D_MONOLITHIC_HEADER ON)   # Used by wrapper code
endif ()

# Implicit configuration
if (ANDROID OR EMSCRIPTEN OR IOS)
    set (URHO3D_SSE OFF)
elseif (URHO3D_TOOLS OR URHO3D_EDITOR)
    set (URHO3D_SYSTEMUI ON)
    set (URHO3D_FILEWATCHER ON)
    set (URHO3D_LOGGING ON)
    set (URHO3D_HASH_DEBUG ON)
endif ()

if (EMSCRIPTEN)
    if (URHO3D_CSHARP)
        message(WARNING "C# is not supported in this configuration.")
        set (URHO3D_CSHARP OFF)
    endif ()
    if (BUILD_SHARED_LIBS)
        set (BUILD_SHARED_LIBS OFF)
        message(WARNING "Shared builds unsupported when compiling with emscripten")     # For now.
    endif ()
    if (NOT URHO3D_PACKAGING)
        # Web builds do not function without data packaging.
        set (URHO3D_PACKAGING ON)
    endif ()
endif ()

if (ANDROID)
    set (SDL_CPUINFO ON)
endif ()

# UWP does not support other framework types.
if (UWP)
    set (URHO3D_NETFX netstandard2.0)
endif ()

# At the end because it depends on URHO3D_SYSTEMUI which is may be off, but implicitly enabled if URHO3D_TOOLS is enabled.
cmake_dependent_option(URHO3D_SYSTEMUI_VIEWPORTS "Use native viewports in supported applications" ON "URHO3D_SYSTEMUI" OFF)
