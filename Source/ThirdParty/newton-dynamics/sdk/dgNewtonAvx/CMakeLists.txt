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

set (projectName "dgNewtonAvx")
#message (${projectName})

# low level core
file(GLOB source *.cpp *.h)

if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fp:fast")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /fp:fast")

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /arch:AVX")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX")
endif()

add_definitions(-DNEWTONCPU_EXPORTS)
add_library(${projectName} SHARED ${source})

if (NEWTON_BUILD_PROFILER)
	target_link_libraries (${projectName} dProfiler)
endif ()

if (MSVC)
	if(CMAKE_VS_MSBUILD_COMMAND OR CMAKE_VS_DEVENV_COMMAND)
		set_target_properties(${projectName} PROPERTIES COMPILE_FLAGS "/YudgNewtonPluginStdafx.h")
		set_source_files_properties(dgNewtonPluginStdafx.cpp PROPERTIES COMPILE_FLAGS "/YcdgNewtonPluginStdafx.h")
	endif()
else()
	target_compile_options(${projectName} PRIVATE -mavx)
endif()

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


add_custom_command(
	TARGET ${projectName} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	ARGS -E copy $<TARGET_FILE:${projectName}> ${PROJECT_BINARY_DIR}/applications/demosSandbox/${CMAKE_CFG_INTDIR}/$<TARGET_FILE_NAME:${projectName}>)


if (NEWTON_BUILD_SANDBOX_DEMOS)
	if (NEWTON_DOUBLE_PRECISION)
		add_custom_command(TARGET ${projectName} POST_BUILD COMMAND ${CMAKE_COMMAND}
						   ARGS -E copy $<TARGET_FILE:${projectName}> ${PROJECT_BINARY_DIR}/applications/demosSandbox/${CMAKE_CFG_INTDIR}/newtonPlugins/${CMAKE_CFG_INTDIR}_double/$<TARGET_FILE_NAME:${projectName}>)
	else ()
		add_custom_command(TARGET ${projectName} POST_BUILD COMMAND ${CMAKE_COMMAND}
						   ARGS -E copy $<TARGET_FILE:${projectName}> ${PROJECT_BINARY_DIR}/applications/demosSandbox/${CMAKE_CFG_INTDIR}/newtonPlugins/${CMAKE_CFG_INTDIR}/$<TARGET_FILE_NAME:${projectName}>)
	endif()
endif ()
