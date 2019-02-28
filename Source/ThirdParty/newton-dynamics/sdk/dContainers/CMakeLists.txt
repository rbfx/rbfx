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

set (projectName "dContainers")
#message (${projectName})

# low level core
file(GLOB CPP_SOURCE *.cpp)
file(GLOB HEADERS *.h)

if(NEWTON_BUILD_SHARED_LIBS)
    if(MSVC)
        add_definitions(-D_DCONTAINERS_DLL)
        add_definitions(-D_DCONTAINERS_EXPORT)
    endif(MSVC)
    add_library(${projectName} SHARED ${CPP_SOURCE})

    target_link_libraries (${projectName} newton)

    if (NEWTON_BUILD_PROFILER)
        target_link_libraries (${projectName} dTimeTracker)
    endif()
else(NEWTON_BUILD_SHARED_LIBS)
    if (MSVC)
        add_definitions(-D_NEWTON_STATIC_LIB)
    endif(MSVC)

    add_library(${projectName} STATIC ${CPP_SOURCE})
endif(NEWTON_BUILD_SHARED_LIBS)

target_link_libraries (${projectName} newton)

if(UNIX)
    target_include_directories(
                        ${projectName} PUBLIC
                        # Headers used from source/build location:
                        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    )
endif(UNIX)

if (MSVC)
	set_target_properties(${projectName} PROPERTIES COMPILE_FLAGS "/YudContainersStdAfx.h")
	set_source_files_properties(dContainersStdAfx.cpp PROPERTIES COMPILE_FLAGS "/YcdContainersStdAfx.h")
	if (NEWTON_BUILD_SANDBOX_DEMOS)
		add_custom_command(
			TARGET ${projectName} POST_BUILD
			COMMAND ${CMAKE_COMMAND}
			ARGS -E copy $(TargetPath) ${PROJECT_BINARY_DIR}/applications/demosSandbox/${CMAKE_CFG_INTDIR}/$(TargetFileName))
	endif ()
endif(MSVC)


target_include_directories(${projectName} PUBLIC 
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
$<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/newton-dynamics/sdk/${projectName}> 
)

if (NOT URHO3D_MERGE_STATIC_LIBS)
install (TARGETS ${projectName} EXPORT Urho3D
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)
endif()

install (DIRECTORY . DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/newton-dynamics/sdk/${projectName} FILES_MATCHING PATTERN *.h)
