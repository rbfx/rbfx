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

if (NOT MINI_URHO)
    # Source environment
    if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
        execute_process(COMMAND cmd /c set OUTPUT_VARIABLE ENVIRONMENT)
    else ()
        execute_process(COMMAND env OUTPUT_VARIABLE ENVIRONMENT)
    endif ()
    string(REGEX REPLACE "=[^\n]*\n?" ";" ENVIRONMENT "${ENVIRONMENT}")
    set(IMPORT_URHO3D_VARIABLES_FROM_ENV BUILD_SHARED_LIBS MINI_URHO SWIG_EXECUTABLE SWIG_DIR)
    foreach(key ${ENVIRONMENT})
        list (FIND IMPORT_URHO3D_VARIABLES_FROM_ENV ${key} _index)
        if ("${key}" MATCHES "^(URHO3D_|CMAKE_|ANDROID_).+" OR ${_index} GREATER -1)
            if (NOT DEFINED ${key})
                set (${key} $ENV{${key}} CACHE STRING "" FORCE)
            endif ()
        endif ()
    endforeach()
endif ()

# Set platform variables
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set (LINUX ON CACHE BOOL "" FORCE)
endif ()

if (ANDROID OR IOS)
    set (MOBILE ON CACHE BOOL "" FORCE)
elseif (APPLE OR "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    set (APPLE ON CACHE BOOL "" FORCE)
endif ()

if (APPLE AND NOT IOS)
    set (MACOS ON CACHE BOOL "" FORCE)
endif ()

if ((WIN32 OR LINUX OR MACOS) AND NOT WEB AND NOT MOBILE)
    set (DESKTOP ON CACHE BOOL "" FORCE)
endif ()

include(CMakeDependentOption)

# Build properties
option(BUILD_SHARED_LIBS                        "Build engine as shared library."       ON)
option(URHO3D_ENABLE_ALL                        "Enable (almost) all engine features."  ON)
option(URHO3D_STATIC_RUNTIME                    "Link to static runtime"               OFF)
set   (URHO3D_SSE             SSE2 CACHE STRING "Enable SSE instructions")

# Subsystems
option                (URHO3D_IK                 "Inverse kinematics subsystem enabled"                  ${URHO3D_ENABLE_ALL})
option                (URHO3D_LOGGING            "Enable logging subsystem"                              ${URHO3D_ENABLE_ALL})
option                (URHO3D_NAVIGATION         "Navigation subsystem enabled"                          ${URHO3D_ENABLE_ALL})
cmake_dependent_option(URHO3D_NETWORK            "Networking subsystem enabled"                          ${URHO3D_ENABLE_ALL} "NOT WEB"                       OFF)
option                (URHO3D_PHYSICS            "Physics subsystem enabled"                             ${URHO3D_ENABLE_ALL})
cmake_dependent_option(URHO3D_PROFILING          "Profiler support enabled"                              ${URHO3D_ENABLE_ALL} "NOT WEB"                       OFF)
option                (URHO3D_SYSTEMUI           "Build SystemUI subsystem"                              ${URHO3D_ENABLE_ALL})
option                (URHO3D_URHO2D             "2D subsystem enabled"                                  ${URHO3D_ENABLE_ALL})

# Features
cmake_dependent_option(URHO3D_CSHARP             "Enable C# support"                                     OFF                  "BUILD_SHARED_LIBS"             OFF)
if (WIN32)
    cmake_dependent_option(URHO3D_WITH_MONO      "Use Mono runtime"                                      OFF                  "URHO3D_CSHARP"                 OFF)
else ()
    set (URHO3D_WITH_MONO ON)
endif ()
cmake_dependent_option(URHO3D_FILEWATCHER        "Watch filesystem for resource changes"                 ${URHO3D_ENABLE_ALL} "URHO3D_THREADING"              OFF)
option                (URHO3D_HASH_DEBUG         "Enable StringHash name debugging"                      ${URHO3D_ENABLE_ALL}                                    )
option                (URHO3D_MONOLITHIC_HEADER  "Create Urho3DAll.h which includes all engine headers." OFF                                                     )
cmake_dependent_option(URHO3D_MINIDUMPS          "Enable writing minidumps on crash"                     ${URHO3D_ENABLE_ALL} "MSVC"                          OFF)
cmake_dependent_option(URHO3D_PLUGINS            "Enable plugins"                                        ${URHO3D_ENABLE_ALL} "BUILD_SHARED_LIBS"             OFF)
cmake_dependent_option(URHO3D_THREADING          "Enable multithreading"                                 ${URHO3D_ENABLE_ALL} "NOT WEB"                       OFF)
option                (URHO3D_WEBP               "WEBP support enabled"                                  ${URHO3D_ENABLE_ALL}                                    )

# Misc
cmake_dependent_option(URHO3D_EXTRAS             "Build extra tools"                                     ${URHO3D_ENABLE_ALL} "NOT WEB AND NOT MOBILE"        OFF)
cmake_dependent_option(URHO3D_TOOLS              "Tools enabled"                                         ${URHO3D_ENABLE_ALL} "NOT WEB AND NOT MOBILE"        OFF)
option                (URHO3D_SAMPLES            "Build samples"                                         OFF)
option                (URHO3D_PACKAGING          "Package resources"                                     OFF)
option                (URHO3D_DOCS               "Build documentation."                                  OFF)
cmake_dependent_option(URHO3D_MERGE_STATIC_LIBS  "Merge third party dependency libs to Urho3D.a"         OFF "NOT BUILD_SHARED_LIBS"                          OFF)

if (WIN32)
    set(URHO3D_RENDERER D3D11 CACHE STRING "Select renderer: D3D9 | D3D11 | OpenGL")
    option(URHO3D_WIN32_CONSOLE "Show log messages in win32 console"                                     OFF                                                     )
elseif (IOS OR ANDROID)
    set(URHO3D_RENDERER GLES2 CACHE STRING "Select renderer: GLES2 | GLES3")
else ()
    set(URHO3D_RENDERER OpenGL)
endif ()
string(TOUPPER "${URHO3D_RENDERER}" URHO3D_RENDERER)
set (URHO3D_${URHO3D_RENDERER} ON)
if (URHO3D_GLES2 OR URHO3D_GLES3)
    set (URHO3D_OPENGL ON)
endif ()

if (URHO3D_CSHARP)
    set (URHO3D_MONOLITHIC_HEADER ON)   # Used by wrapper code
endif ()

# Implicit configuration
if (ANDROID OR WEB OR IOS)
    set (URHO3D_SSE OFF)
    if (WEB)
        set (EMSCRIPTEN_MEMORY_LIMIT 128 CACHE NUMBER "Memory limit in megabytes. Set to 0 for dynamic growth.")
        option (EMSCRIPTEN_MEMORY_GROWTH "Allow memory growth. Disables some optimizations." OFF)
    endif ()
elseif (URHO3D_TOOLS)
    set (URHO3D_SYSTEMUI ON)
    set (URHO3D_FILEWATCHER ON)
    set (URHO3D_LOGGING ON)
    set (URHO3D_HASH_DEBUG ON)
endif ()

if (WEB)
    set (URHO3D_PACKAGING ON)
    if (URHO3D_CSHARP)
        message(WARNING "Web builds do not support C#.")
        set (URHO3D_CSHARP OFF)
    endif ()
endif ()
