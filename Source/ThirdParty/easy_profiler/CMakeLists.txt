cmake_minimum_required(VERSION 3.0)
project(easy_profiler CXX)

set(EASY_PROGRAM_VERSION_MAJOR 1)
set(EASY_PROGRAM_VERSION_MINOR 3)
set(EASY_PROGRAM_VERSION_PATCH 0)
set(EASY_PRODUCT_VERSION_STRING "${EASY_PROGRAM_VERSION_MAJOR}.${EASY_PROGRAM_VERSION_MINOR}.${EASY_PROGRAM_VERSION_PATCH}")

if (IOS)
    set(BUILD_WITH_CHRONO_HIGH_RESOLUTION_CLOCK ON)
endif()

set (CMAKE_CXX_STANDARD 11)
add_subdirectory(easy_profiler_core)

find_package(Qt5Widgets QUIET)
if (Qt5Widgets_FOUND AND URHO3D_TOOLS)
    # Only include the Qt client if on we're building desktop platform and Qt5 was found on system
    add_subdirectory(profiler_gui)
endif()
