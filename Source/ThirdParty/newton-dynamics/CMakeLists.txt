# Copyright (c) <2014-2017> <Newton Game Dynamics>
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely.

cmake_minimum_required(VERSION 3.10.0)




option("NEWTON_BUILD_SANDBOX_DEMOS" "generates demos projects" ON)
option("NEWTON_BUILD_PROFILER" "build profiler" OFF)
option("NEWTON_BUILD_SINGLE_THREADED" "multi threaded" OFF)
option("NEWTON_DOUBLE_PRECISION" "Generate double precision" OFF)
option("NEWTON_STATIC_RUNTIME_LIBRARIES" "use windows static libraries" ON)
#option("NEWTON_WITH_SSE_PLUGIN" "adding asse parallel solver" OFF)
option("NEWTON_WITH_SSE4_PLUGIN" "adding avx parallel solver" OFF)
option("NEWTON_WITH_AVX_PLUGIN" "adding avx parallel solver" ON)
option("NEWTON_WITH_AVX2_PLUGIN" "adding avx parallel solver" OFF)
#option("NEWTON_WITH_DX12_PLUGIN" "adding direct compute 12 parallel solver" OFF)
option("NEWTON_BUILD_SHARED_LIBS" "Build shared library" OFF)

message("NEWTON_STATIC_RUNTIME_LIBRARIES: " ${NEWTON_STATIC_RUNTIME_LIBRARIES} )
message("NEWTON_WITH_AVX_PLUGIN: " ${NEWTON_WITH_AVX_PLUGIN} )
message("NEWTON_WITH_SSE_PLUGIN: " ${NEWTON_WITH_SSE_PLUGIN} )




set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo)

#Urho Doesnt Do "_d"'s
#set(CMAKE_DEBUG_POSTFIX "_d")


project(NewtonSDK)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
if (NEWTON_DOUBLE_PRECISION)
	add_definitions(-D_NEWTON_USE_DOUBLE)
endif ()

if (NEWTON_BUILD_SINGLE_THREADED)
	add_definitions(-DDG_USE_THREAD_EMULATION)
endif ()

#If no build type set, Release as default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
          "Choose the type of build, options are: Debug Release"
          FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# determine if we are compiling for a 32bit or 64bit system
include(CheckTypeSize)
CHECK_TYPE_SIZE("void*" PTR_SIZE BUILTIN_TYPES_ONLY)
if (PTR_SIZE EQUAL 8)
  set(BUILD_64 TRUE)
else ()
  set(BUILD_64 FALSE)
endif ()

#check for MAC
if(APPLE)
  add_definitions(-D_MACOSX_VER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions -std=c++11")
endif(APPLE)

#check for UNIX
if (UNIX)
    if (BUILD_64)
      add_definitions(-D_POSIX_VER_64)
    else (BUILD_64)
      add_definitions(-D_POSIX_VER)
    endif (BUILD_64)
endif(UNIX)

if (UNIX)
    include("GNUInstallDirs")


    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

    add_definitions(-march=native -fPIC)

    set(NEWTON_GENERATE_DLL OFF CACHE BOOL "" FORCE)
    set(NEWTON_STATIC_RUNTIME_LIBRARIES OFF CACHE BOOL "" FORCE)
    set(NEWTON_WITH_AVX_PLUGIN OFF CACHE BOOL "" FORCE)
    set(NEWTON_BUILD_PROFILER OFF CACHE BOOL "" FORCE)

else (UNIX)
    set(CMAKE_INSTALL_LIBDIR "${PROJECT_BINARY_DIR}/build/lib")
    set(CMAKE_INSTALL_BINDIR "${PROJECT_BINARY_DIR}/build/bin")
    set(CMAKE_INSTALL_INCLUDEDIR "${PROJECT_BINARY_DIR}/build/include")
endif (UNIX)

if (MSVC)
	#message ("build newton visual studio projects")

	if(NEWTON_STATIC_RUNTIME_LIBRARIES)
		message ("link to static runtime libraries")
		set(CMAKE_C_FLAGS_DEBUG "/MTd")
		set(CMAKE_C_FLAGS_RELEASE "/MT")
		set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	else()
		#message ("newton link to dynamic runtime libraries")
		set(CMAKE_C_FLAGS_DEBUG "/MDd")
		set(CMAKE_C_FLAGS_RELEASE "/MD")
		set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
		set(CMAKE_CXX_FLAGS_RELEASE "/MD")
	endif()

	if(CMAKE_CL_64)
		#message ("build 64 bit")
		add_definitions(-D_WIN_64_VER)
	else()
		#message ("build 32 bit")
		add_definitions(-D_WIN_32_VER)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE2")
	endif()

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /W4 /Od")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /GS- /W4 /Ox /Oi /Ot /Ob2 /fp:fast")

	message("CMAKE_CXX_FLAGS_DEBUG is ${CMAKE_CXX_FLAGS_DEBUG}")
	message("CMAKE_CXX_FLAGS_RELEASE is ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

add_subdirectory(sdk)


add_dependencies (dgPhysics dgCore)
add_dependencies (dContainers dMath)
add_dependencies (newton dgCore dgPhysics)
add_dependencies (dAnimation dMath dContainers)
add_dependencies (dCustomJoints dMath dContainers)
add_dependencies (dNewton newton dMath dContainers dCustomJoints)
add_dependencies (dVehicle newton dMath dContainers dCustomJoints)

if (NEWTON_BUILD_PROFILER)
	add_definitions(-D_DG_USE_PROFILER)
        add_dependencies (newton dTimeTracker)
        add_dependencies (dNewton dTimeTracker)
        add_dependencies (dVehicle dTimeTracker)
        add_dependencies (dAnimation dTimeTracker)
        add_dependencies (dContainers dTimeTracker)
        add_dependencies (dCustomJoints dTimeTracker)
endif ()

