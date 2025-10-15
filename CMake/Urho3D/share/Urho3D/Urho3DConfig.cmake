#
# Copyright (c) 2025-2025 the rbfx project.
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
# This is a dummy package configuration file for Urho3D, which allows consuming
# engine through in-source builds (through `add_subdirectory()`) by using same
# `find_package()` mechanism like using SDK.
#

# Source root directory
get_filename_component(rbfx_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

# Include engine as in-source build
add_subdirectory(${rbfx_SOURCE_DIR} ${CMAKE_BINARY_DIR}/rbfx)

# Make engine modules accessible
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${rbfx_SOURCE_DIR}/CMake/Modules)

# Include common functionality
include(UrhoCommon)

# Mark Urho3D as found
set(Urho3D_FOUND TRUE)
