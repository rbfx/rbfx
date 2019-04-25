#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(EASTL)

#-------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------
option(EASTL_BUILD_BENCHMARK "Enable generation of build files for benchmark" OFF)
option(EASTL_BUILD_TESTS "Enable generation of build files for tests" OFF)

#-------------------------------------------------------------------------------------------
# Sub-projects
#-------------------------------------------------------------------------------------------
add_subdirectory(test/packages/EABase)

if(EASTL_BUILD_TESTS OR EASTL_BUILD_BENCHMARK)
    add_subdirectory(test/packages/EAAssert)
    add_subdirectory(test/packages/EAStdC)
    add_subdirectory(test/packages/EAMain)
    add_subdirectory(test/packages/EATest)
    add_subdirectory(test/packages/EAThread)
endif()

if(EASTL_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

if(EASTL_BUILD_TESTS)
    add_subdirectory(test)
endif()

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------
add_definitions(-D_CHAR16T)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DEASTL_OPENSOURCE=1)

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
file(GLOB EASTL_SOURCES "source/*.cpp" "include/EASTL/*.h" "include/EASTL/bonus/*.h")
add_library(EASTL STATIC ${EASTL_SOURCES})
#include_directories("include")

#-------------------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------------------
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/scripts/CMake")
include(CommonCppFlags)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND
        (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel"
            OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo") AND MINGW)
    message(FATAL_ERROR "FIXME: 32bits mingw-w64 clang crashes when compiling with any optimizations.")
endif()

#-------------------------------------------------------------------------------------------
# Include dirs
#-------------------------------------------------------------------------------------------
target_include_directories(EASTL
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EABase/include/Common>
)

#-------------------------------------------------------------------------------------------
# Libraries
#-------------------------------------------------------------------------------------------
target_compile_definitions(EASTL PUBLIC -Deastl=stl -DEASTL_RTTI_ENABLED=0 -DEASTL_URHO3D_EXTENSIONS=1)
# Urho3D uses `unsigned` for sizes all over place. Can not use native size without breaking lots of code.
target_compile_definitions(EASTL PUBLIC -DEASTL_SIZE_T_32BIT=1)

add_subdirectory(test/packages/EAStdC)      # Provides some sprintf implementations
add_subdirectory(test/packages/EAAssert)
target_link_libraries(EASTL PUBLIC EAStdC)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(EASTL PUBLIC -DEASTL_DLL)
endif ()

# Urho3D:
install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
install (DIRECTORY test/packages/EABase/include/Common/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS EASTL EAStdC EABase EAAssert EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})
endif ()
