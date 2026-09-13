# Copyright (c) 2008-2022 the Urho3D project.
# Copyright (c) 2022-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# Don't include XR sample if we don't have XR on.
if (NOT URHO3D_XR)
    return ()
endif ()

file (GLOB SAMPLE_CODE *.h *.cpp)
list (APPEND SOURCE_CODE ${SAMPLE_CODE})
set (SOURCE_CODE "${SOURCE_CODE}" PARENT_SCOPE)
