# Copyright (c) 2024-2024 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT URHO3D_TESTING)
    return ()
endif ()

file (GLOB_RECURSE TEST_SOURCE_CODE RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp *.h)

# Group source code in VS solution
group_sources()

set (TARGET_NAME Benchmarks)
add_executable(${TARGET_NAME} ${TEST_SOURCE_CODE})

target_link_libraries(${TARGET_NAME} PRIVATE Urho3D benchmark::benchmark)
