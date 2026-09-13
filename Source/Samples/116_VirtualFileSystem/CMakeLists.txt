# Copyright (c) 2023-2023 the Urho3D project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

file (GLOB SAMPLE_CODE *.h *.cpp)
list (APPEND SOURCE_CODE ${SAMPLE_CODE})
set (SOURCE_CODE "${SOURCE_CODE}" PARENT_SCOPE)
