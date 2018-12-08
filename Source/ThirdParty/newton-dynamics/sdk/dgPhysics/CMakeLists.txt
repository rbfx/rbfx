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

set (projectName "dgPhysics")
#message (${projectName})

# low level core
file(GLOB CPP_SOURCE *.cpp ../dgMeshUtil/*.cpp)
file(GLOB HEADERS *.h ../dgMeshUtil/*.h)

if (MSVC)
       set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:fast")
       set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast")
endif(MSVC)

if (UNIX)
	if(NEWTON_BUILD_SHARED_LIBS)
	   add_library(${projectName} SHARED ${CPP_SOURCE})
	else(NEWTON_BUILD_SHARED_LIBS)
	   add_library(${projectName} STATIC ${CPP_SOURCE})
	endif(NEWTON_BUILD_SHARED_LIBS)

        target_include_directories(
                         ${projectName} PUBLIC
                         # Headers used from source/build location:
                         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        )
endif(UNIX)

if (MSVC)
   add_library(${projectName} STATIC ${CPP_SOURCE})
	set_target_properties(${projectName} PROPERTIES COMPILE_FLAGS "/YudgPhysicsStdafx.h")
	set_source_files_properties(dgWorld.cpp PROPERTIES COMPILE_FLAGS "/YcdgPhysicsStdafx.h")
endif(MSVC)


target_include_directories(${projectName} PUBLIC 
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
$<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/sdk> 
)


install (TARGETS ${projectName} EXPORT Urho3D
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)