#-------------------------------------------------------------------------------------------
# Copyright (C) Electronic Arts Inc.  All rights reserved.
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.11)
include(FetchContent)
project(EASTL CXX)

#-------------------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------------------
option(EASTL_BUILD_BENCHMARK "Enable generation of build files for benchmark" OFF)
option(EASTL_BUILD_TESTS "Enable generation of build files for tests" OFF)
option(EASTL_STD_ITERATOR_CATEGORY_ENABLED "Enable compatibility with std:: iterator categories" OFF)


option(EASTL_DISABLE_APRIL_2024_DEPRECATIONS "Enable use of API marked for removal in April 2024." OFF)
option(EASTL_DISABLE_SEPT_2024_DEPRECATIONS "Enable use of API marked for removal in September 2024." OFF)
option(EASTL_DISABLE_APRIL_2025_DEPRECATIONS "Enable use of API marked for removal in April 2025." OFF)

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
# Library definition
#-------------------------------------------------------------------------------------------
file(GLOB EASTL_SOURCES "source/*.cpp")
file(GLOB_RECURSE EASTL_HEADERS "include/EASTL/**.h")
add_library(EASTL STATIC ${EASTL_SOURCES} ${EASTL_HEADERS})
target_compile_features(EASTL PUBLIC cxx_std_14)

# include both source and headers in the files tab in Visual Studio
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${EASTL_HEADERS})

if (MSVC)
    set(EASTL_NATVIS_DIR "doc")
    set(EASTL_NATVIS_FILE "${EASTL_NATVIS_DIR}/EASTL.natvis")
    target_sources(EASTL INTERFACE
        $<INSTALL_INTERFACE:${EASTL_NATVIS_FILE}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${EASTL_NATVIS_FILE}>
    )
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
target_compile_definitions(EASTL
    PUBLIC  -DEASTL_OPENSOURCE=1
            -DEASTL_RTTI_ENABLED=0
            -DEASTL_SIZE_T_32BIT=1        # Urho3D uses `unsigned` for sizes all over place. Can not use native size without breaking lots of code.
            -DEASTDC_GLOBALPTR_SUPPORT_ENABLED=0
            -DEASTDC_THREADING_SUPPORTED=0
            -DEASTL_STD_ITERATOR_CATEGORY_ENABLED=1
            -DEASTL_DEBUG=$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>
	    -D_CHAR16T=1
)
if (MSVC)
    target_compile_definitions(EASTL PRIVATE -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
elseif (GCC)
    target_compile_options(EASTL PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>)
endif()

if (EASTL_STD_ITERATOR_CATEGORY_ENABLED)
  add_definitions(-DEASTL_STD_ITERATOR_CATEGORY_ENABLED=1)
endif()

if (BUILD_SHARED_LIBS)
    target_compile_definitions(EASTL PUBLIC -DEA_DLL=1 PRIVATE -DEASTL_EXPORTS=1 -DEASTDC_EXPORTS=1 -DEAASSERT_EXPORTS=1)
endif ()

if (UWP)
    target_compile_options(EASTL PUBLIC /Zc:twoPhase-)
endif ()

#-------------------------------------------------------------------------------------------
# Include dirs
#-------------------------------------------------------------------------------------------
target_include_directories(EASTL SYSTEM PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

#-------------------------------------------------------------------------------------------
# Dependencies
#-------------------------------------------------------------------------------------------
set(EABase_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EABase)
set(EAAssert_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EAAssert)
set(EAStdC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EAStdC)

file(GLOB_RECURSE EAASSERT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EAAssert/source/*.cpp")
target_sources(EASTL PRIVATE ${EAASSERT_SOURCES})

file(GLOB_RECURSE EASTDC_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/packages/EAStdC/source/*.cpp")
target_sources(EASTL PRIVATE ${EASTDC_SOURCES})

target_include_directories(EASTL SYSTEM
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${EABase_SOURCE_DIR}/include/Common>
           $<BUILD_INTERFACE:${EAAssert_SOURCE_DIR}/include>
           $<BUILD_INTERFACE:${EAStdC_SOURCE_DIR}/include>
)

#-------------------------------------------------------------------------------------------
# Deprecations
#-------------------------------------------------------------------------------------------
if(EASTL_DISABLE_APRIL_2024_DEPRECATIONS)
  target_compile_definitions(EASTL PUBLIC EA_DEPRECATIONS_FOR_2024_APRIL=EA_DISABLED)
endif()
if(EASTL_DISABLE_SEPT_2024_DEPRECATIONS)
  target_compile_definitions(EASTL PUBLIC EA_DEPRECATIONS_FOR_2024_SEPT=EA_DISABLED)
endif()
if(EASTL_DISABLE_APRIL_2025_DEPRECATIONS)
  target_compile_definitions(EASTL PUBLIC EA_DEPRECATIONS_FOR_2025_APRIL=EA_DISABLED)
endif()


#-------------------------------------------------------------------------------------------
# Installation
#-------------------------------------------------------------------------------------------
install (DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ FILES_MATCHING PATTERN *.h)
install (DIRECTORY ${EABase_SOURCE_DIR}/include/Common/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ FILES_MATCHING PATTERN *.h)
install (DIRECTORY ${EAAssert_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ FILES_MATCHING PATTERN *.h)
install (DIRECTORY ${EAStdC_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ FILES_MATCHING PATTERN *.h)
if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS EASTL EXPORT Urho3D ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()
