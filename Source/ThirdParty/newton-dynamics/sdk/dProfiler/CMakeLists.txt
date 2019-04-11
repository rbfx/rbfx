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

set (projectName "dProfiler")
message (${projectName})

# low level core
file(GLOB HEADERS *.h)
#file(GLOB CPP_SOURCE *.cpp)
file(GLOB CPP_SOURCE *.cpp ../thirdParty/tracy/TracyClient.cpp)

add_definitions(-DD_PROFILER_EXPORTS)

if (NOT MSVC OR MSVC_VERSION VERSION_GREATER 1800)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4245 /wd4127 /wd4091 /wd4130 /wd4189 /wd4244 /wd4267 /wd4324 /wd4389 /wd4456 /wd4701 /wd4702 /wd4800 /wd4996")
	include_directories(../thirdParty/tracy/)
	add_definitions(-DTRACY_ENABLE)
endif()

add_library(${projectName} SHARED ${CPP_SOURCE})
target_include_directories(${projectName} PUBLIC .)

if (MSVC)
	target_link_libraries (${projectName} ws2_32.lib)
endif(MSVC)

install(TARGETS ${projectName}
       LIBRARY DESTINATION lib
       ARCHIVE DESTINATION lib
       RUNTIME DESTINATION bin)

install(FILES ${HEADERS} DESTINATION include/${projectName})

if (NEWTON_BUILD_SANDBOX_DEMOS)
	add_custom_command(
		TARGET ${projectName} POST_BUILD
		COMMAND ${CMAKE_COMMAND}
		ARGS -E copy $<TARGET_FILE:${projectName}> ${PROJECT_BINARY_DIR}/applications/demosSandbox/${CMAKE_CFG_INTDIR}/$<TARGET_FILE_NAME:${projectName}>)
endif ()
