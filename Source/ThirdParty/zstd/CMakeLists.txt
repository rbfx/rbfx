#
# Copyright (c) 2025 the rbfx project.
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
# CMake build script for zstd library
# Uses the single-file amalgamation (zstd.c)
# See https://github.com/facebook/zstd/tree/dev/build/single_file_libs

cmake_minimum_required(VERSION 3.10)
project(zstd C)

# Create zstd library from the single-file amalgamation
add_library(zstd STATIC src/zstd.c)

# Create alias for compatibility with Tracy and other projects
add_library(libzstd ALIAS zstd)
add_library(libzstd_static ALIAS zstd)

# Set include directories
target_include_directories(zstd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/zstd>
)

# Define ZSTD_STATIC_LINKING_ONLY if building static library
target_compile_definitions(zstd PUBLIC ZSTD_STATIC_LINKING_ONLY)

# Set properties
set_target_properties(zstd PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)

# Export headers for installation
file(GLOB ZSTD_PUBLIC_HEADERS *.h)
install(FILES ${ZSTD_PUBLIC_HEADERS} DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/zstd)
