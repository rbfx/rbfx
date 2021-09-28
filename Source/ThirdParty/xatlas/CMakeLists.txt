#cmake_minimum_required(VERSION 2.8.8)
project(xatlas)

# Urho3D: Removed all extra things
add_library(xatlas STATIC xatlas.cpp)

if (NOT MINI_URHO)
    if (NOT URHO3D_MERGE_STATIC_LIBS)
        install(TARGETS xatlas EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})
    endif ()
endif ()

target_include_directories(xatlas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
