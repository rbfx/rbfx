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
set(IMPORT_URHO3D_VARIABLES_FROM_ENV BUILD_SHARED_LIBS MINI_URHO SWIG_EXECUTABLE SWIG_DIR)
foreach(key ${ENVIRONMENT})
    list (FIND IMPORT_URHO3D_VARIABLES_FROM_ENV ${key} _index)
    if ("${key}" MATCHES "^(URHO3D_|CMAKE_|ANDROID_).+" OR ${_index} GREATER -1)
        if (NOT DEFINED ${key})
            set (${key} $ENV{${key}} CACHE STRING "" FORCE)
        endif ()
    endif ()
endforeach()

# Set platform variables
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set (LINUX ON)
endif ()

if (ANDROID OR IOS)
    set (MOBILE ON)
elseif (APPLE OR "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    set (APPLE ON)
endif ()

if (APPLE AND NOT IOS)
    set (MACOS ON)
endif ()

if ((WIN32 OR LINUX OR MACOS) AND NOT WEB AND NOT MOBILE)
    set (DESKTOP ON)
endif ()

option(URHO3D_ENABLE_ALL "Enables all optional subsystems by default" OFF)

# Determine library type
if (BUILD_SHARED_LIBS)
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

# Enable features user has chosen
foreach(FEATURE in ${URHO3D_FEATURES})
    set (URHO3D_${FEATURE} ON CACHE BOOL "" FORCE)
endforeach()

macro (_option NAME DESCRIPTION)
    if (NOT ${NAME}_DEFAULT)
        set (${NAME}_DEFAULT ${URHO3D_ENABLE_ALL})
    endif ()
    if (NOT DEFINED ${NAME})
        option(${NAME} "${DESCRIPTION}" ${${NAME}_DEFAULT})
    endif ()
endmacro ()

# Enable by default for developers preset
if (URHO3D_DEVELOPER)
    foreach (OPT in PROFILING TOOLS EXTRAS LOGGING SYSTEMUI FILEWATCHER HASH_DEBUG)
        if (NOT DEFINED URHO3D_${OPT}_DEFAULT)
            set (URHO3D_${OPT}_DEFAULT ${URHO3D_DEVELOPER})
        endif ()
    endforeach ()
endif ()

# Options
option(URHO3D_STATIC_RUNTIME         "Enable link to static runtime" OFF)
option(URHO3D_THIRDPARTY_SHARED_LIBS "Build third party dependencies as static libraries" ${BUILD_SHARED_LIBS})

_option(URHO3D_IK "Inverse kinematics subsystem enabled")
_option(URHO3D_NAVIGATION "Navigation subsystem enabled")
_option(URHO3D_PHYSICS "Physics subsystem enabled")
_option(URHO3D_URHO2D "2D subsystem enabled")
_option(URHO3D_WEBP "WEBP support enabled")
_option(URHO3D_NETWORK "Networking subsystem enabled")
_option(URHO3D_PROFILING "Profiler support enabled")
_option(URHO3D_THREADING "Enable multithreading")
_option(URHO3D_TOOLS "Tools enabled")
_option(URHO3D_EXTRAS "Build extra tools")
_option(URHO3D_SSE "Enable SSE instructions")
_option(URHO3D_SAMPLES "Build samples")
_option(URHO3D_LOGGING "Enable logging subsystem")
_option(URHO3D_SYSTEMUI "Build SystemUI subsystem")
_option(URHO3D_PACKAGING "Package resources")
_option(URHO3D_FILEWATCHER "Watch filesystem for resource changes")
_option(URHO3D_TASKS "Enable coroutine subsystem")
_option(URHO3D_HASH_DEBUG "Enable StringHash name debugging")
_option(URHO3D_PLUGINS "Enable native plugins" ${URHO3D_ENABLE_ALL})
_option(URHO3D_MONOLITHIC_HEADER "Create Urho3DAll.h which includes all engine headers." OFF)
if (BUILD_SHARED_LIBS)
    _option(URHO3D_CSHARP "Enable C# support")
else ()
    if (URHO3D_CSHARP)
        message (WARNING "CSharp is supported only when building with -DBUILD_SHARED_LIBS=ON")
    endif ()
    set (URHO3D_CSHARP OFF)
endif ()
if (MSVC)
    _option(URHO3D_MINIDUMPS "Enable writing minidumps on crash")
else ()
    set (URHO3D_MINIDUMPS OFF)
endif ()
if (URHO3D_CSHARP)
    if (WIN32)
        option(URHO3D_WITH_MONO "Use Mono runtime" OFF)
    else ()
        set (URHO3D_WITH_MONO ON)
    endif ()
else ()
    set (URHO3D_WITH_MONO OFF)
endif ()

if (WIN32)
    set(URHO3D_RENDERER D3D11 CACHE STRING "Select renderer: D3D9 | D3D11 | OpenGL")
else ()
    set(URHO3D_RENDERER OpenGL)
endif ()
string(TOUPPER "${URHO3D_RENDERER}" URHO3D_RENDERER)
set (URHO3D_${URHO3D_RENDERER} ON)

# Implicit configuration
if (ANDROID OR WEB OR IOS)
    set (URHO3D_SSE OFF)
    set (URHO3D_TOOLS OFF)
    set (URHO3D_TASKS OFF)
    if (WEB)
        set (URHO3D_NETWORK OFF)         # Not supported by kNet
        set (URHO3D_PROFILING OFF)       # No way to make use of profiler data because of lack of networking
        set (URHO3D_TOOLS OFF)           # Useless
        set (URHO3D_EXTRAS OFF)          # Useless
        set (EMSCRIPTEN_MEMORY_LIMIT 128 CACHE NUMBER "Memory limit in megabytes. Set to 0 for dynamic growth.")
        option (EMSCRIPTEN_MEMORY_GROWTH "Allow memory growth. Disables some optimizations." OFF)
    endif ()
else ()
    if (URHO3D_TOOLS)
        set (URHO3D_SYSTEMUI ON)
        set (URHO3D_FILEWATCHER ON)
        set (URHO3D_LOGGING ON)
        if (NOT DEFINED URHO3D_HASH_DEBUG)
            set (URHO3D_HASH_DEBUG ON)
        elseif (NOT URHO3D_HASH_DEBUG)
            message (WARNING "Editor uses URHO3D_HASH_DEBUG feature, but it was disabled. Editor will still build but some features will break.")
        endif ()
    endif ()
endif ()

if (APPLE)
    set (URHO3D_PLUGINS OFF)
endif ()

if (WEB)
    if (BUILD_SHARED_LIBS)
        message(FATAL_ERROR "Web builds are supported only with static linking.")
    endif ()
    if (NOT DEFINED URHO3D_PACKAGING)
        set (URHO3D_PACKAGING ON)
    endif ()
    if (URHO3D_CSHARP)
        message(WARNING "Web builds do not support C#.")
        set (URHO3D_CSHARP OFF)
    endif ()
    set (URHO3D_TOOLS OFF)
    set (URHO3D_EXTRAS OFF)
    set (URHO3D_THREADING OFF)
    set (URHO3D_NETWORK OFF)
endif ()

# Unset any default config variables so they do not pollute namespace
get_cmake_property(__cmake_variables VARIABLES)
foreach (var ${__cmake_variables})
    if ("${var}" MATCHES "^URHO3D_.*_DEFAULT")
        if (${${var}})
            unset(${var})
        endif ()
    endif ()
endforeach()
