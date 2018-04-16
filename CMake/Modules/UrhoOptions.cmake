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
set(IMPORT_URHO3D_VARIABLES_FROM_ENV BUILD_SHARED_LIBS MINI_URHO)
foreach(key ${ENVIRONMENT})
    list (FIND IMPORT_URHO3D_VARIABLES_FROM_ENV ${key} _index)
    if ("${key}" MATCHES "^(URHO3D_|CMAKE_|ANDROID_).+" OR ${_index} GREATER -1)
        if (NOT DEFINED ${key})
            set (${key} $ENV{${key}} CACHE STRING "" FORCE)
        endif ()
    endif ()
endforeach()

# Set platform variables
if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    set (LINUX ON)
endif ()

if (ANDROID OR IOS)
    set (MOBILE ON)
elseif (APPLE OR "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    set (APPLE ON)
endif ()

if (WIN32 OR LINUX OR MACOS)
    set (DESKTOP ON)
endif ()

option(URHO3D_ENABLE_ALL "Enables all optional subsystems by default" OFF)

# Determine library type
string(TOUPPER "${BUILD_SHARED_LIBS}" BUILD_SHARED_LIBS)
if ("${BUILD_SHARED_LIBS}" STREQUAL "MODULE")
    set (BUILD_SHARED_LIBS OFF)
    set (URHO3D_LIBRARY_TYPE MODULE)
elseif (BUILD_SHARED_LIBS)
    set (URHO3D_LIBRARY_TYPE SHARED)
else ()
    set (URHO3D_LIBRARY_TYPE STATIC)
endif ()

# Threads are still experimental on emscripten.
if (NOT EMSCRIPTEN OR URHO3D_ENABLE_ALL)
    set (URHO3D_THREADS_DEFAULT ON)
else ()
    set (URHO3D_THREADS_DEFAULT OFF)
endif ()

# Configuration presets
set(URHO3D_PRESET None CACHE STRING "Select configuration preset: None | Developer | Release")
string(TOUPPER "${URHO3D_PRESET}" URHO3D_PRESET)

if ("${URHO3D_PRESET}" STREQUAL "DEVELOPER")
    set (URHO3D_DEVELOPER ON)
    set (URHO3D_RELEASE OFF)
elseif ("${URHO3D_PRESET}" STREQUAL "RELEASE")
    set (URHO3D_DEVELOPER OFF)
    set (URHO3D_RELEASE ON)
else ()
    set (URHO3D_DEVELOPER ${URHO3D_ENABLE_ALL})
    set (URHO3D_RELEASE ${URHO3D_ENABLE_ALL})
endif ()

set (URHO3D_PROFILING_DEFAULT ${URHO3D_DEVELOPER})
set (URHO3D_TOOLS_DEFAULT ${URHO3D_DEVELOPER})
set (URHO3D_EXTRAS_DEFAULT ${URHO3D_DEVELOPER})
set (URHO3D_LOGGING_DEFAULT ${URHO3D_DEVELOPER})
set (URHO3D_SYSTEMUI_DEFAULT ${URHO3D_DEVELOPER})
set (URHO3D_FILEWATCHER_DEFAULT ${URHO3D_DEVELOPER})

option(URHO3D_IK "Inverse kinematics subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_NAVIGATION "Navigation subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_PHYSICS "Physics subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_URHO2D "2D subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_WEBP "WEBP support enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_NETWORK "Networking subsystem enabled" ${URHO3D_ENABLE_ALL})
option(URHO3D_PROFILING "Profiler support enabled" ${URHO3D_PROFILING_DEFAULT})
option(URHO3D_THREADING "Enable multithreading" ${URHO3D_THREADS_DEFAULT})
if (ANDROID OR WEB OR IOS)
    set (URHO3D_TOOLS OFF)
else ()
    option(URHO3D_TOOLS "Tools enabled" ${URHO3D_TOOLS_DEFAULT})
endif ()
option(URHO3D_STATIC_RUNTIME "Enable link to static runtime" OFF)
option(URHO3D_EXTRAS "Build extra tools" ${URHO3D_EXTRAS_DEFAULT})
option(URHO3D_SSE "Enable SSE instructions" ${URHO3D_ENABLE_ALL})
option(URHO3D_SAMPLES "Build samples" ${URHO3D_ENABLE_ALL})
option(URHO3D_LOGGING "Enable logging subsystem" ${URHO3D_LOGGING_DEFAULT})
option(URHO3D_SYSTEMUI "Build SystemUI subsystem" ${URHO3D_DEVELOPER})
option(URHO3D_PACKAGING "Package resources" ${URHO3D_RELEASE})
option(URHO3D_FILEWATCHER "Watch filesystem for resource changes" ${URHO3D_DEVELOPER})
option(URHO3D_CSHARP "Enable C# support" ${URHO3D_ENABLE_ALL})
if (IOS OR ANDROID OR WEB)
    set (URHO3D_TASKS OFF)
else ()
    option(URHO3D_TASKS "Enable coroutine subsystem" ${URHO3D_ENABLE_ALL})
endif ()

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

if (EMSCRIPTEN)
    set(URHO3D_NETWORK OFF)         # Not supported by kNet
    set(URHO3D_PROFILING OFF)       # No way to make use of profiler data because of lack of networking
    set(URHO3D_TOOLS OFF)           # Useless
    set(URHO3D_EXTRAS OFF)          # Useless
    set(URHO3D_SSE OFF)             # Unsupported
    set(EMSCRIPTEN_MEMORY_LIMIT 128 CACHE NUMBER "Memory limit in megabytes. Set to 0 for dynamic growth.")
    option(EMSCRIPTEN_MEMORY_GROWTH "Allow memory growth. Disables some optimizations." OFF)
endif ()

if (ANDROID)
    set(URHO3D_SSE OFF)
endif ()

# Unset any default config variables so they do not pollute namespace
get_cmake_property(__cmake_variables VARIABLES)
foreach (var ${__cmake_variables})
    if ("${var}" MATCHES "^URHO3D_.*_DEFAULT")
        if (${${var}})
            set(${var})
        endif ()
    endif ()
endforeach()
