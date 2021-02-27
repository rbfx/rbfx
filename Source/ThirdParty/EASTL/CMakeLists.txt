#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.1)
project(EASTL)

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)   # rbfx
endif ()

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/scripts/CMake")
include(CommonCppFlags)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND
   (CMAKE_BUILD_TYPE MATCHES "Release" OR CMAKE_BUILD_TYPE MATCHES "MinSizeRel"
    OR CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo") AND MINGW)
    message(FATAL_ERROR "FIXME: 32bits mingw-w64 clang crashes when compiling with any optimizations.")
endif()

file(GLOB_RECURSE EASTL_SOURCES
    source/*.cpp
    include/EASTL/*.h
    test/packages/EAAssert/include/*.h
    test/packages/EAAssert/include/*.cpp
    test/packages/EABase/include/*.h
    test/packages/EAStdC/include/*.h
    test/packages/EAStdC/source/*.cpp
)

add_library(EASTL STATIC ${EASTL_SOURCES} doc/EASTL.natvis)
target_compile_definitions(EASTL
    PUBLIC  -DEASTL_OPENSOURCE=1
            -DEASTL_RTTI_ENABLED=0
            -DEASTL_URHO3D_EXTENSIONS=1
            -DEASTL_SIZE_T_32BIT=1        # Urho3D uses `unsigned` for sizes all over place. Can not use native size without breaking lots of code.
            -DEASTDC_GLOBALPTR_SUPPORT_ENABLED=0
            -DEASTDC_THREADING_SUPPORTED=0
            -DEASTL_STD_ITERATOR_CATEGORY_ENABLED=1
    PRIVATE -D_CHAR16T
)
if (MSVC)
    target_compile_definitions(EASTL PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
elseif (GCC)
    target_compile_options(EASTL PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>)
endif ()

target_include_directories(EASTL SYSTEM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EABase/include/Common>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EAAssert/include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EAStdC/include>
)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(EASTL PUBLIC -DEA_DLL PRIVATE -DEASTL_EXPORTS -DEASTDC_EXPORTS)
endif ()

# Urho3D:
if (UWP)
    target_compile_options(EASTL PUBLIC /Zc:twoPhase-)
endif ()
if (NOT MINI_URHO)
    install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
    install (DIRECTORY test/packages/EABase/include/Common/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
    install (DIRECTORY test/packages/EAAssert/include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
    install (DIRECTORY test/packages/EAStdC/include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)
    if (NOT URHO3D_MERGE_STATIC_LIBS)
        install(TARGETS EASTL EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
    endif ()
endif ()
