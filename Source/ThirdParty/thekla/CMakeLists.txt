cmake_minimum_required(VERSION 2.8.8)
project(thekla_atlas)

include_directories(
    src
    src/nvcore
    extern/tinyobj
    extern/poshlib)

# Urho3D: Disable shared libs
set (NVCORE_SHARED OFF)
set (NVIMAGE_SHARED OFF)
set (NVMATH_SHARED OFF)
set (NVMESH_SHARED OFF)

add_subdirectory(extern/poshlib)
add_subdirectory(src/nvcore)
add_subdirectory(src/nvimage)
add_subdirectory(src/nvmath)
add_subdirectory(src/nvmesh)

# Urho3D: Removed all extra things
add_library(libthekla_atlas STATIC
    src/thekla/thekla_atlas.cpp
)

target_link_libraries(libthekla_atlas nvmesh)

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS libthekla_atlas EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})
endif ()

target_include_directories(libthekla_atlas PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
