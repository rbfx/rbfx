#
# Copyright (c) 2017-2025 the rbfx project.
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

# Function to generate platform tag: <platform>-<architecture>-<runtime>
# Supported platforms:
#   - windows
#   - uwp
#   - linux
#   - macos
#   - ios
#   - tvos
#   - android
#   - emscripten
#
# Supported architectures:
#   - x86
#   - x64
#   - arm
#   - arm64
#   - wasm (emscripten only)
#
# Supported runtimes:
#   - dll (shared libraries)
#   - lib (static libraries)
#
# Example tags:
#   - emscripten-wasm-lib
#   - windows-x64-dll
#   - uwp-arm64-dll
#   - uwp-arm-lib
#   - linux-x86-lib
#   - android-arm64-lib
#
# Arguments:
#   OUT_TAG - Output variable name for the tag
#   MODE - Optional: "HOST" for host platform, "TARGET" (default) for target platform
function(rbfx_get_platform_tag OUT_TAG)
    # Parse optional MODE argument
    set(MODE "TARGET")
    if(ARGC GREATER 1)
        set(MODE "${ARGV1}")
    endif()

    # Select appropriate CMake variables based on mode
    if(MODE STREQUAL "HOST")
        set(SYSTEM_NAME "${CMAKE_HOST_SYSTEM_NAME}")
        set(SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    else()
        set(SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
        if(NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "")
            set(SYSTEM_PROCESSOR "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
        elseif(NOT CMAKE_GENERATOR_PLATFORM STREQUAL "")
            set(SYSTEM_PROCESSOR "${CMAKE_GENERATOR_PLATFORM}")
        elseif(APPLE AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "" AND NOT CMAKE_OSX_ARCHITECTURES MATCHES ";")
            # TODO: Support universal binaries with multiple architectures
            set(SYSTEM_PROCESSOR "${CMAKE_OSX_ARCHITECTURES}")
        elseif(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "")
            set(SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
        else()
            set(SYSTEM_PROCESSOR "${CMAKE_HOST_SYSTEM_PROCESSOR}")
        endif()
    endif()

    # Detect platform
    set(PLATFORM_NAME "unknown")
    if(MODE STREQUAL "HOST")
        # Host platform detection
        if(CMAKE_HOST_WIN32)
            set(PLATFORM_NAME "windows")
        elseif(CMAKE_HOST_APPLE)
            set(PLATFORM_NAME "macos")
        elseif(CMAKE_HOST_UNIX)
            set(PLATFORM_NAME "linux")
        endif()
    else()
        # Target platform detection
        if(WIN32)
            if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
                set(PLATFORM_NAME "uwp")
            else()
                set(PLATFORM_NAME "windows")
            endif()
        elseif(ANDROID)
            set(PLATFORM_NAME "android")
        elseif(EMSCRIPTEN)
            set(PLATFORM_NAME "emscripten")
        elseif(APPLE)
            if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
                set(PLATFORM_NAME "macos")
            elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
                set(PLATFORM_NAME "ios")
            elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
                set(PLATFORM_NAME "tvos")
            endif()
        elseif(UNIX)
            set(PLATFORM_NAME "linux")
        endif()
    endif()

    # Detect architecture and normalize
    set(ARCH "${SYSTEM_PROCESSOR}")
    string(TOLOWER "${ARCH}" ARCH)
    if(ARCH MATCHES "^(x86_64|amd64|x64)$")
        set(ARCH "x64")
    elseif(ARCH MATCHES "^(i386|i486|i586|i686|x86|win32)$")
        set(ARCH "x86")
    elseif(ARCH MATCHES "^(aarch64|arm64|arm64e|arm64_32|arm64ec)$")
        set(ARCH "arm64")
    elseif(ARCH MATCHES "^(arm|armv4i|armv5i|armv5|armv6|armv7|armv7k|armv7s)$")
        set(ARCH "arm")
    elseif(EMSCRIPTEN AND MODE STREQUAL "TARGET")
        set(ARCH "wasm")
    endif()

    if(ARCH STREQUAL "")
        message(STATUS "CMAKE_HOST_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME}")
        message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR ${CMAKE_HOST_SYSTEM_PROCESSOR}")
        message(STATUS "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
        message(STATUS "CMAKE_CXX_COMPILER_ARCHITECTURE_ID ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
        message(FATAL_ERROR "Unsupported architecture")
    endif()

    # Detect runtime linkage
    # For both HOST and TARGET modes, check BUILD_SHARED_LIBS
    # This ensures that host tools built with dll match host tools consumed with dll
    if(DEFINED BUILD_SHARED_LIBS AND BUILD_SHARED_LIBS)
        set(RUNTIME "dll")
    else()
        set(RUNTIME "lib")
    endif()

    # Construct the tag
    set(${OUT_TAG} "${PLATFORM_NAME}-${ARCH}-${RUNTIME}" PARENT_SCOPE)
endfunction()

# Function to check if a tool tag is compatible with the host tag
# This performs a "relaxed" check suitable for tools/executables:
#   - Platform must match exactly (windows, linux, macos, etc.)
#   - Architecture is relaxed: x86 tools can run on x64/arm64, arm tools can run on arm64
#   - Runtime (dll/lib) is ignored for tools
#
# Arguments:
#   OUT_COMPATIBLE - Output variable name for compatibility result (TRUE/FALSE)
#   HOST_TAG - The host platform tag (where tools will run)
#   TOOL_TAG - The tool's platform tag (what it was built for)
function(rbfx_check_tool_tag_compatibility OUT_COMPATIBLE HOST_TAG TOOL_TAG)
    # Parse tags: platform-arch-runtime
    string(REPLACE "-" ";" HOST_PARTS "${HOST_TAG}")
    string(REPLACE "-" ";" TOOL_PARTS "${TOOL_TAG}")

    list(LENGTH HOST_PARTS HOST_PARTS_COUNT)
    list(LENGTH TOOL_PARTS TOOL_PARTS_COUNT)

    if(HOST_PARTS_COUNT LESS 2 OR TOOL_PARTS_COUNT LESS 2)
        # Invalid tag format
        set(${OUT_COMPATIBLE} FALSE PARENT_SCOPE)
        return()
    endif()

    list(GET HOST_PARTS 0 HOST_PLATFORM)
    list(GET HOST_PARTS 1 HOST_ARCH)
    list(GET TOOL_PARTS 0 TOOL_PLATFORM)
    list(GET TOOL_PARTS 1 TOOL_ARCH)

    # Platform must match exactly
    if(NOT HOST_PLATFORM STREQUAL TOOL_PLATFORM)
        set(${OUT_COMPATIBLE} FALSE PARENT_SCOPE)
        return()
    endif()

    # Architecture compatibility check (relaxed)
    # Exact match is always compatible
    if(HOST_ARCH STREQUAL TOOL_ARCH)
        set(${OUT_COMPATIBLE} TRUE PARENT_SCOPE)
        return()
    endif()

    # x86 tools can run on x64 or arm64 hosts (through emulation/compatibility layers)
    if(TOOL_ARCH STREQUAL "x86" AND HOST_ARCH STREQUAL "x64")
        set(${OUT_COMPATIBLE} TRUE PARENT_SCOPE)
        return()
    endif()

    # arm (32-bit) tools can run on arm64 hosts
    if(TOOL_ARCH STREQUAL "arm" AND HOST_ARCH STREQUAL "arm64")
        set(${OUT_COMPATIBLE} TRUE PARENT_SCOPE)
        return()
    endif()

    # No other combinations are compatible
    set(${OUT_COMPATIBLE} FALSE PARENT_SCOPE)
endfunction()
