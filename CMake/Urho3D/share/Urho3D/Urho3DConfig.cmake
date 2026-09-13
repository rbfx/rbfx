# Copyright (c) 2025-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# This is a dummy package configuration file for Urho3D, which allows consuming
# engine through in-source builds (through `add_subdirectory()`) by using same
# `find_package()` mechanism like using SDK.

# Source root directory
get_filename_component(rbfx_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

# Include engine as in-source build
add_subdirectory(${rbfx_SOURCE_DIR} ${CMAKE_BINARY_DIR}/rbfx)

# Make engine modules accessible
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${rbfx_SOURCE_DIR}/CMake/Modules)

# Include common functionality
include(UrhoCommon)

# Provide path to package root
set(Urho3D_PACKAGE_ROOT "${rbfx_SOURCE_DIR}")

# Mark Urho3D as found
set(Urho3D_FOUND TRUE)
