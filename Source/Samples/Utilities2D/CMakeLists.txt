# Copyright (c) 2017-2022 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT URHO3D_URHO2D)
    return ()
endif ()

file(GLOB SOURCE_FILES *.h *.cpp)
add_library(Utilities2D STATIC ${SOURCE_FILES})
target_link_libraries(Utilities2D Urho3D)
target_include_directories(Utilities2D PUBLIC ..)
