#
# Copyright (c) 2008-2020 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

include(ucm)
include(VSSolution)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CLANG ON)
    set (GNU ON)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (GCC ON)
    set (GNU ON)
endif()

# Set compiler variable
set ("${CMAKE_CXX_COMPILER_ID}" ON)
if (NOT WEB)
    set (CMAKE_INSTALL_RPATH "$ORIGIN")
    if (GNU OR MINGW)
        # Reduce size of debugging information in RelWithDebInfo builds.
        ucm_replace_flag(-g -g1 CONFIG RelWithDebInfo)
        ucm_replace_flag(-O2 -Og CONFIG RelWithDebInfo)
    endif ()
endif ()

# Configure variables
set (URHO3D_URL "https://github.com/urho3d/Urho3D")
set (URHO3D_DESCRIPTION "Urho3D is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE (http://www.ogre3d.org) and Horde3D (http://www.horde3d.org).")
execute_process (COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE URHO3D_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
string (REGEX MATCH "([^.]+)\\.([^.]+)\\.(.+)" MATCHED "${URHO3D_VERSION}")
set(RBFX_CSPROJ_LIST "" CACHE STRING "A list of C# projects." FORCE)

# Setup SDK install destinations
if (WIN32)
    set (SCRIPT_EXT .bat)
else ()
    set (SCRIPT_EXT .sh)
endif ()
if (ANDROID)
    # For Android platform, install to a path based on the chosen Android ABI, e.g. libs/armeabi-v7a
    set (LIB_SUFFIX s/${ANDROID_NDK_ABI_NAME})
endif ()
set (DEST_BASE_INCLUDE_DIR include)
set (DEST_INCLUDE_DIR ${DEST_BASE_INCLUDE_DIR}/Urho3D)
set (DEST_BIN_DIR bin)
set (DEST_SHARE_DIR share)
set (DEST_RESOURCE_DIR ${DEST_BIN_DIR})
set (DEST_BUNDLE_DIR ${DEST_SHARE_DIR}/Applications)
set (DEST_ARCHIVE_DIR lib)
set (DEST_PKGCONFIG_DIR ${DEST_ARCHIVE_DIR}/pkgconfig)
set (DEST_THIRDPARTY_HEADERS_DIR ${DEST_INCLUDE_DIR}/ThirdParty)
if (ANDROID)
    set (DEST_LIBRARY_DIR ${DEST_ARCHIVE_DIR})
else ()
    set (DEST_LIBRARY_DIR bin)
endif ()

if (MSVC OR "${CMAKE_GENERATOR}" STREQUAL "Xcode")
    set (MULTI_CONFIG_PROJECT ON)
endif ()
if (MULTI_CONFIG_PROJECT)
    set (DEST_BIN_DIR_CONFIG ${DEST_BIN_DIR}/$<CONFIG>)
    set (DEST_LIBRARY_DIR_CONFIG ${DEST_LIBRARY_DIR}/$<CONFIG>)
    set (DEST_ARCHIVE_DIR_CONFIG ${DEST_ARCHIVE_DIR}/$<CONFIG>)
else ()
    set (DEST_BIN_DIR_CONFIG ${DEST_BIN_DIR})
    set (DEST_LIBRARY_DIR_CONFIG ${DEST_LIBRARY_DIR})
    set (DEST_ARCHIVE_DIR_CONFIG ${DEST_ARCHIVE_DIR})
endif ()
if (WIN32)
    set (WINVER 0x0601)
    add_definitions(-DWINVER=${WINVER} -D_WIN32_WINNT=${WINVER} -D_WIN32_WINDOWS=${WINVER})
endif ()
if (NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND MACOS)
    set (CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
endif ()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_ARCHIVE_DIR})

set (VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if (NOT DEFINED URHO3D_64BIT)
    if (CMAKE_SIZEOF_VOID_P MATCHES 8)
        set(URHO3D_64BIT ON)
        set(URHO3D_PLATFORM x64)
    else ()
        set(URHO3D_64BIT OFF)
        set(URHO3D_PLATFORM x86)
    endif ()
endif ()

if (MINGW)
    find_file(DLL_FILE_PATH_1 "libstdc++-6.dll")
    find_file(DLL_FILE_PATH_2 "libgcc_s_seh-1.dll")
    find_file(DLL_FILE_PATH_3 "libwinpthread-1.dll")
    foreach (DLL_FILE_PATH ${DLL_FILE_PATH_1} ${DLL_FILE_PATH_2} ${DLL_FILE_PATH_3})
        if (DLL_FILE_PATH)
            # Copies dlls to bin or tools.
            file (COPY ${DLL_FILE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG})
            if (NOT URHO3D_STATIC_RUNTIME)
                file (COPY ${DLL_FILE_PATH} DESTINATION ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG})
            endif ()
        endif ()
    endforeach ()
endif ()

# Configure for web
if (WEB)
    # Emscripten-specific setup
    if (EMSCRIPTEN_EMCC_VERSION VERSION_LESS 1.31.3)
        message(FATAL_ERROR "Unsupported compiler version")
    endif ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths -Wno-unknown-warning-option")
    if (URHO3D_THREADING)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_PTHREADS=1")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_PTHREADS=1")
    endif ()
    set (CMAKE_C_FLAGS_RELEASE "-Oz -DNDEBUG")
    set (CMAKE_CXX_FLAGS_RELEASE "-Oz -DNDEBUG")
    # Remove variables to make the -O3 regalloc easier, embed data in asm.js to reduce number of moving part
    set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1 --memory-init-file 0")
    set (CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1 --memory-init-file 0")
    # Preserve LLVM debug information, show line number debug comments, and generate source maps; always disable exception handling codegen
    set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
    set (CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} -g4 -s DISABLE_EXCEPTION_CATCHING=1")
endif ()

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
    set (HOST_LINUX 1)
elseif ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set (HOST_WIN32 1)
elseif ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Darwin")
    set (HOST_MACOS 1)
endif ()

if (NOT APPLE AND CLANG)
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.1)
        # Workaround for Clang 7.0.1 and above until the Bullet upstream has fixed the Clang 7 diagnostic checks issue (see https://github.com/bulletphysics/bullet3/issues/2114)
        ucm_add_flags(-Wno-argument-outside-range)
    endif ()
endif ()

if (URHO3D_SSE)
    if (DESKTOP)
        string (TOLOWER "${URHO3D_SSE}" URHO3D_SSE)
        if (MSVC)
            list (APPEND VALID_SSE_OPTIONS sse sse2 avx avx2)
        else ()
            list (APPEND VALID_SSE_OPTIONS sse sse2 sse3 sse4 sse4a sse4.1 sse4.2 avx avx2)
        endif ()
        list (FIND VALID_SSE_OPTIONS "${URHO3D_SSE}" SSE_INDEX)
        if (SSE_INDEX EQUAL -1)
            set (URHO3D_SSE sse2)
        endif ()
        if (MSVC)
            string (TOUPPER "${URHO3D_SSE}" URHO3D_SSE)
        endif ()
    else ()
        set (URHO3D_SSE OFF)
    endif ()
endif ()

if (MINGW AND CROSS_TARGET)
    # Workarounds for crosscompiling MinGW on linux. EASTL uses uppser-case headers where MinGW has them lower-case.
    foreach(HEADER Windows.h WinSock2.h ShellAPI.h XInput.h Rpc.h)
        find_path(${HEADER}_PATH ${HEADER})
        if (NOT ${${HEADER}_PATH})
            string(TOLOWER "${HEADER}" HEADER_LOWER)
            find_path(${HEADER}_PATH ${HEADER_LOWER})
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes)
            execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${${HEADER}_PATH}/${HEADER_LOWER} ${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes/${HEADER})
        endif ()
    endforeach()
    include_directories(${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes)
endif ()

# Macro for setting symbolic link on platform that supports it
function (create_symlink SOURCE DESTINATION)
    # Make absolute paths so they work more reliably on cmake-gui
    if (IS_ABSOLUTE ${SOURCE})
        set (ABS_SOURCE ${SOURCE})
    else ()
        set (ABS_SOURCE ${CMAKE_SOURCE_DIR}/${SOURCE})
    endif ()
    if (IS_ABSOLUTE ${DESTINATION})
        set (ABS_DESTINATION ${DESTINATION})
    else ()
        set (ABS_DESTINATION ${CMAKE_BINARY_DIR}/${DESTINATION})
    endif ()
    if (CMAKE_HOST_WIN32)
        if (IS_DIRECTORY ${ABS_SOURCE})
            set (SLASH_D /D)
        else ()
            unset (SLASH_D)
        endif ()
        set (RESULT_CODE 1)
        if(${CMAKE_SYSTEM_VERSION} GREATER_EQUAL 6.0)
            if (NOT EXISTS ${ABS_DESTINATION})
                # Have to use string-REPLACE as file-TO_NATIVE_PATH does not work as expected with MinGW on "backward slash" host system
                string (REPLACE / \\ BACKWARD_ABS_DESTINATION ${ABS_DESTINATION})
                string (REPLACE / \\ BACKWARD_ABS_SOURCE ${ABS_SOURCE})
                execute_process (COMMAND cmd /C mklink ${SLASH_D} ${BACKWARD_ABS_DESTINATION} ${BACKWARD_ABS_SOURCE} OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE RESULT_CODE)
            endif ()
        endif ()
        if (NOT "${RESULT_CODE}" STREQUAL "0")
            if (SLASH_D)
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_directory ${ABS_SOURCE} ${ABS_DESTINATION})
            else ()
                set (COMMAND COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABS_SOURCE} ${ABS_DESTINATION})
            endif ()
            # Fallback to copy only one time
            if (NOT EXISTS ${ABS_DESTINATION})
                execute_process (${COMMAND})
            endif ()
        endif ()
    else ()
        execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink ${ABS_SOURCE} ${ABS_DESTINATION})
    endif ()
endfunction ()

# Groups sources into subfolders.
macro(group_sources)
    file (GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/**)
    foreach (child ${children})
        if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
            string(REPLACE "/" "\\" groupname "${child}")
            file (GLOB files LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/${child}/*)
            source_group(${groupname} FILES ${files})
        endif ()
    endforeach ()
endmacro()

macro (initialize_submodule SUBMODULE_DIR)
    file(GLOB SUBMODULE_FILES ${SUBMODULE_DIR}/*)
    list(LENGTH SUBMODULE_FILES SUBMODULE_FILES_LEN)

    if (SUBMODULE_FILES_LEN LESS 2)
        find_program(GIT git)
        if (NOT GIT)
            message(FATAL_ERROR "git not found.")
        endif ()
        execute_process(
            COMMAND git submodule update --init --remote "${SUBMODULE_DIR}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE SUBMODULE_RESULT
        )
        if (NOT SUBMODULE_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to initialize submodule ${SUBMODULE_DIR}")
        endif ()
    endif ()
endmacro ()

function(vs_group_subdirectory_targets DIRECTORY FOLDER_NAME)
    cmake_parse_arguments(GRP "FORCE" "" "" ${ARGN})
    get_property(DIRS DIRECTORY ${DIRECTORY} PROPERTY SUBDIRECTORIES)
    foreach(DIR ${DIRS})
        get_property(TARGETS DIRECTORY ${DIR} PROPERTY BUILDSYSTEM_TARGETS)
        foreach(TARGET ${TARGETS})
            get_target_property(TARGET_TYPE ${TARGET} TYPE)
            if (NOT ${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY")
                get_target_property(CURRENT_FOLDER ${TARGET} FOLDER)
                if (NOT CURRENT_FOLDER OR GRP_FORCE)
                    set_target_properties(${TARGET} PROPERTIES FOLDER ${FOLDER_NAME})
                endif ()
            endif ()
        endforeach()
        vs_group_subdirectory_targets(${DIR} ${FOLDER_NAME} ${ARGN})
    endforeach()
endfunction()

if (URHO3D_CSHARP)
    if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
        # Workaround for some cases where csc has issues when invoked by CMake.
        set (TERM_WORKAROUND env TERM=xterm)
    endif ()
    if (CMAKE_VS_MSBUILD_COMMAND)
        set (MSBUILD ${CMAKE_VS_MSBUILD_COMMAND} CACHE STRING "")
    else ()
        find_program(MSBUILD msbuild HINTS $<MACOS:/Library/Frameworks/Mono.framework/Versions/Current/bin> $ENV{PATH})
    endif ()

    if (NOT MSBUILD)
        message(FATAL_ERROR "msbuild executable was not found.")
    endif ()

    if (URHO3D_NETFX MATCHES "net(46|standard2).*")
        set (URHO3D_NETFX_LEGACY_VERSION ON)
    endif ()

    if (WIN32)
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER win7-${URHO3D_PLATFORM})
    elseif (MACOS)
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER osx-${URHO3D_PLATFORM})
    else ()
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER linux-${URHO3D_PLATFORM})
    endif ()

    if (URHO3D_OMNISHARP_HELPER)
        message(STATUS "Omnisharp helper saves .OmniSharp.props to ${rbfx_SOURCE_DIR}.")
        message(STATUS "This helper helps OmniSharp to find CMAKE_BINARY_DIR.")
        message(STATUS "As a consequence - you must reconfigure project in order to use this source tree with another cmake binary dir!")
        file(WRITE "${rbfx_SOURCE_DIR}/.OmniSharp.props" "<Project ToolsVersion=\"15.0\"><PropertyGroup><CMAKE_BINARY_DIR>${CMAKE_BINARY_DIR}</CMAKE_BINARY_DIR></PropertyGroup></Project>")
    else ()
        file(REMOVE "${rbfx_SOURCE_DIR}/.OmniSharp.props")
    endif ()
endif ()

function (add_msbuild_target)
    if (URHO3D_CSHARP)
        cmake_parse_arguments(MSBUILD "EXCLUDE_FROM_ALL" "TARGET;DEPENDS" "ARGS;BYPRODUCTS" ${ARGN})
        add_custom_target(${MSBUILD_TARGET} ALL
            COMMAND ${TERM_WORKAROUND} ${MSBUILD} ${MSBUILD_ARGS}
            /p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/
            /consoleloggerparameters:ErrorsOnly /nologo
            BYPRODUCTS ${MSBUILD_BYPRODUCTS}
            DEPENDS ${MSBUILD_DEPENDS})
        if (MSBUILD_EXCLUDE_FROM_ALL)
            set_target_properties(${MSBUILD_TARGET} PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
        endif ()
    endif ()
endfunction ()

if (URHO3D_CSHARP)
    # For .csproj embedded into visual studio solution
    configure_file("${rbfx_SOURCE_DIR}/CMake/CMake.props.in" "${CMAKE_BINARY_DIR}/CMake.props" @ONLY)
    # For .csproj that gets built by cmake invoking msbuild
    set (ENV{CMAKE_GENERATOR} "${CMAKE_GENERATOR}")
    set (ENV{CMAKE_BINARY_DIR "${CMAKE_BINARY_DIR}/")
    set (ENV{RBFX_BINARY_DIR "${rbfx_BINARY_DIR}/")
    set (ENV{CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")

    set (DOTNET_FRAMEWORK_TYPES net46 net461 net462 net47 net471 net472 net48)
    set (DOTNET_FRAMEWORK_VERSIONS v4.6 v4.6.1 v4.6.2 v4.7 v4.7.1 v4.7.2 v4.8)
    list (FIND DOTNET_FRAMEWORK_TYPES ${URHO3D_NETFX} DOTNET_FRAMEWORK_INDEX)
    if (DOTNET_FRAMEWORK_INDEX GREATER -1)
        list (GET DOTNET_FRAMEWORK_VERSIONS ${DOTNET_FRAMEWORK_INDEX} CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION)
    endif ()
    unset (DOTNET_FRAMEWORK_INDEX)

    if (NOT SWIG_LIB)
        set (SWIG_DIR ${rbfx_SOURCE_DIR}/Source/ThirdParty/swig/Lib)
    endif ()
    include(UrhoSWIG)
endif()

macro (__TARGET_GET_PROPERTIES_RECURSIVE OUTPUT TARGET PROPERTY)
    get_target_property(values ${TARGET} ${PROPERTY})
    if (values)
        list (APPEND ${OUTPUT} ${values})
    endif ()
    get_target_property(values ${TARGET} INTERFACE_LINK_LIBRARIES)
    if (values)
        foreach(lib ${values})
            if (TARGET ${lib} AND NOT "${lib}" IN_LIST __CHECKED_LIBRARIES)
                list (APPEND __CHECKED_LIBRARIES "${lib}")
                __TARGET_GET_PROPERTIES_RECURSIVE(${OUTPUT} ${lib} ${PROPERTY})
            endif ()
        endforeach()
    endif()
endmacro()

function (add_target_csharp)
    cmake_parse_arguments (CS "EXE" "TARGET;PROJECT;OUTPUT" "DEPENDS" ${ARGN})
    if (MSVC)
        include_external_msproject(${CS_TARGET} ${CS_PROJECT} TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC ${CS_DEPENDS})
        if (CS_DEPENDS)
            add_dependencies(${CS_TARGET} ${CS_DEPENDS})
        endif ()
    else ()
        if (MULTI_CONFIG_PROJECT)
            set (CSHARP_CONFIG $<CONFIG>)
        else ()
            set (CSHARP_CONFIG ${CMAKE_BUILD_TYPE})
        endif ()
        add_msbuild_target(TARGET ${CS_TARGET} DEPENDS ${CS_DEPENDS} ARGS ${CS_PROJECT}
            /p:Platform=${URHO3D_PLATFORM}
            /p:Configuration=${CSHARP_CONFIG}
            BYPRODUCTS ${CS_OUTPUT_FILE}
        )
        list(APPEND RBFX_CSPROJ_LIST ${CS_PROJECT})
        set(RBFX_CSPROJ_LIST "${RBFX_CSPROJ_LIST}" CACHE STRING "A list of C# projects." FORCE)
    endif ()
endfunction ()

function (csharp_bind_target)
    if (NOT URHO3D_CSHARP)
        return ()
    endif ()

    cmake_parse_arguments(BIND "" "TARGET;CSPROJ;SWIG;NAMESPACE;NATIVE" "INCLUDE_DIRS" ${ARGN})

    get_target_property(BIND_SOURCE_DIR ${BIND_TARGET} SOURCE_DIR)

    # Parse bindings using same compile definitions as built target
    __TARGET_GET_PROPERTIES_RECURSIVE(INCLUDES ${BIND_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    __TARGET_GET_PROPERTIES_RECURSIVE(DEFINES  ${BIND_TARGET} INTERFACE_COMPILE_DEFINITIONS)
    __TARGET_GET_PROPERTIES_RECURSIVE(OPTIONS  ${BIND_TARGET} INTERFACE_COMPILE_OPTIONS)
    if (INCLUDES)
        list (REMOVE_DUPLICATES INCLUDES)
    endif ()
    if (DEFINES)
        list (REMOVE_DUPLICATES DEFINES)
    endif ()
    if (OPTIONS)
        list (REMOVE_DUPLICATES OPTIONS)
    endif ()
    foreach(item ${INCLUDES})
        if ("${item}" MATCHES "\\$<INSTALL_INTERFACE:.+")
            continue()
        endif ()
        if ("${item}" MATCHES "\\$<BUILD_INTERFACE:.+")
            string(LENGTH "${item}" len)
            math(EXPR len "${len} - 19")
            string(SUBSTRING "${item}" 18 ${len} item)
        endif ()
        list(APPEND GENERATOR_OPTIONS -I${item})
    endforeach()
    foreach(item ${DEFINES})
        string(FIND "${item}" "=" EQUALITY_INDEX)
        if (EQUALITY_INDEX EQUAL -1)
            set (item "${item}=1")
        endif ()
        list(APPEND GENERATOR_OPTIONS "-D\"${item}\"")
    endforeach()

    if (URHO3D_NETFX_LEGACY_VERSION)
        # TODO: Not great, little bit terrible.
        list(APPEND GENERATOR_OPTIONS "-D\"URHO3D_NETFX_LEGACY_VERSION=1\"")
    endif ()

    if (NOT BIND_NATIVE)
        set (BIND_NATIVE ${BIND_TARGET})
    endif ()

    # Swig
    if (IOS)
        list (APPEND GENERATOR_OPTIONS -D__IOS__)
    endif ()
    set(CMAKE_SWIG_FLAGS
        -namespace ${BIND_NAMESPACE}
        -fastdispatch
        -I${CMAKE_CURRENT_BINARY_DIR}
        ${GENERATOR_OPTIONS}
    )

    foreach(item ${OPTIONS})
        list(APPEND GENERATOR_OPTIONS -O${item})
    endforeach()

    # Finalize option list
    list (APPEND GENERATOR_OPTIONS ${BIND_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Swig)
    set (CSHARP_BINDING_GENERATOR_OPTIONS "${CMAKE_CURRENT_BINARY_DIR}/generator_options_${BIND_TARGET}.txt")
    file (WRITE ${CSHARP_BINDING_GENERATOR_OPTIONS} "")
    foreach (opt ${GENERATOR_OPTIONS})
        file(APPEND ${CSHARP_BINDING_GENERATOR_OPTIONS} "${opt}\n")
    endforeach ()

    set (SWIG_SYSTEM_INCLUDE_DIRS "${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES};${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES};${CMAKE_SYSTEM_INCLUDE_PATH};${CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_INCLUDE_DIRS}")
    if (BIND_INCLUDE_DIRS)
        list (APPEND SWIG_SYSTEM_INCLUDE_DIRS ${BIND_INCLUDE_DIRS})
    endif ()
    string (REPLACE ";" ";-I" SWIG_SYSTEM_INCLUDE_DIRS "${SWIG_SYSTEM_INCLUDE_DIRS}")

    set_source_files_properties(${BIND_SWIG} PROPERTIES
        CPLUSPLUS ON
        SWIG_FLAGS "-I${SWIG_SYSTEM_INCLUDE_DIRS}"
    )
    if (NOT DEFINED SWIG_MODULE_${BIND_TARGET}_DLLIMPORT)
        if (IOS)
            set (SWIG_MODULE_${BIND_TARGET}_DLLIMPORT __Internal)
        else ()
            set (SWIG_MODULE_${BIND_TARGET}_DLLIMPORT ${BIND_NATIVE})
        endif ()
    endif ()
    set (SWIG_MODULE_${BIND_TARGET}_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/${BIND_TARGET}CSharp)
    set (SWIG_MODULE_${BIND_TARGET}_NO_LIBRARY ON)

    # CMakeCache.txt does not exist when cache directory is configured for the first time and when user changes any
    # cmake parameters. We exploit this to delete bindings upon configuration change and force their regeneration.
    if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
        file(REMOVE_RECURSE ${SWIG_MODULE_${BIND_TARGET}_OUTDIR})
    endif ()

    swig_add_module(${BIND_TARGET} csharp ${BIND_SWIG})
    unset (CMAKE_SWIG_FLAGS)

    if (MULTI_CONFIG_PROJECT)
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    else ()
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        # Needed for mono on unixes but not on windows.
        set (FACADES Facades/)
    endif ()
    if (BIND_CSPROJ)
        get_filename_component(BIND_MANAGED_TARGET "${BIND_CSPROJ}" NAME_WE)
        add_target_csharp(
            TARGET ${BIND_MANAGED_TARGET}
            PROJECT ${BIND_CSPROJ}
            OUTPUT ${NET_OUTPUT_DIRECTORY}/${BIND_MANAGED_TARGET}.dll)
        add_dependencies(${BIND_MANAGED_TARGET} ${BIND_TARGET})
        install (FILES ${NET_OUTPUT_DIRECTORY}/${BIND_MANAGED_TARGET}.dll DESTINATION ${DEST_LIBRARY_DIR_CONFIG})
    endif ()
endfunction ()

function (create_pak PAK_DIR PAK_FILE)
    cmake_parse_arguments(PARSE_ARGV 2 PAK "NO_COMPRESS" "" "DEPENDS")

    get_filename_component(NAME "${PAK_FILE}" NAME)
    if (CMAKE_CROSSCOMPILING)
        set (DEPENDENCY Urho3D-Native)
    else ()
        set (DEPENDENCY PackageTool)
    endif ()

    if (NOT PAK_NO_COMPRESS)
        list (APPEND PAK_FLAGS -c)
    endif ()

    set_property (SOURCE ${PAK_FILE} PROPERTY GENERATED TRUE)
    add_custom_command(
        OUTPUT "${PAK_FILE}"
        COMMAND "${PACKAGE_TOOL}" "${PAK_DIR}" "${PAK_FILE}" -q ${PAK_FLAGS}
        DEPENDS ${DEPENDENCY} ${PAK_DEPENDS}
        COMMENT "Packaging ${NAME}"
    )
endfunction ()

macro(web_executable TARGET)
    if (WEB)
        set_target_properties (${TARGET} PROPERTIES SUFFIX .html)
        if (EMSCRIPTEN_MEMORY_LIMIT)
            math(EXPR EMSCRIPTEN_TOTAL_MEMORY "${EMSCRIPTEN_MEMORY_LIMIT} * 1024 * 1024")
            target_link_libraries(${TARGET} PRIVATE "-s TOTAL_MEMORY=${EMSCRIPTEN_TOTAL_MEMORY}")
        endif ()
        if (EMSCRIPTEN_MEMORY_GROWTH)
            target_link_libraries(${TARGET} PRIVATE "-s ALLOW_MEMORY_GROWTH=1")
        endif ()
        target_link_libraries(${TARGET} PRIVATE "-s NO_EXIT_RUNTIME=1" "-s FORCE_FILESYSTEM=1")
        if (BUILD_SHARED_LIBS)
            target_link_libraries(${TARGET} PRIVATE "-s MAIN_MODULE=1")
        endif ()
    endif ()
endmacro()

function (package_resources_web)
    if (NOT WEB)
        return ()
    endif ()

    cmake_parse_arguments(PAK "" "RELATIVE_DIR;OUTPUT;INSTALL_TO" "FILES" ${ARGN})
    if (NOT "${PAK_RELATIVE_DIR}" MATCHES "/$")
        set (PAK_RELATIVE_DIR "${PAK_RELATIVE_DIR}/")
    endif ()
    string(LENGTH "${PAK_RELATIVE_DIR}" PAK_RELATIVE_DIR_LEN)

    foreach (file ${PAK_FILES})
        string(SUBSTRING "${file}" ${PAK_RELATIVE_DIR_LEN} 999999 rel_file)
        set (PRELOAD_FILES ${PRELOAD_FILES} ${file}@${rel_file})
    endforeach ()

    # TODO: Do we need it? It breaks debug build.
    # See https://github.com/emscripten-core/emscripten/issues/10555
    #if (CMAKE_BUILD_TYPE STREQUAL Debug AND EMSCRIPTEN_EMCC_VERSION VERSION_GREATER 1.32.2)
    #    set (SEPARATE_METADATA --separate-metadata)
    #endif ()
    get_filename_component(LOADER_DIR "${PAK_OUTPUT}" DIRECTORY)
    add_custom_target("${PAK_OUTPUT}"
        COMMAND ${EMPACKAGER} ${PAK_RELATIVE_DIR}${PAK_OUTPUT}.data --preload ${PRELOAD_FILES} --js-output=${PAK_RELATIVE_DIR}${PAK_OUTPUT} --use-preload-cache ${SEPARATE_METADATA}
        SOURCES ${PAK_FILES}
        DEPENDS ${PAK_FILES}
        COMMENT "Packaging ${PAK_OUTPUT}"
    )
    if (CMAKE_CROSSCOMPILING)
        add_dependencies("${PAK_OUTPUT}" Urho3D-Native)
    else ()
        add_dependencies("${PAK_OUTPUT}" PackageTool)
    endif ()
    if (PAK_INSTALL_TO)
        install(FILES "${PAK_RELATIVE_DIR}${PAK_OUTPUT}" "${PAK_RELATIVE_DIR}${PAK_OUTPUT}.data" DESTINATION ${PAK_INSTALL_TO})
    endif ()
endfunction ()

function (web_link_resources TARGET RESOURCES)
    if (NOT WEB)
        return ()
    endif ()
    file (WRITE "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCES}.load.js" "var Module;if(typeof Module==='undefined')Module=eval('(function(){try{return Module||{}}catch(e){return{}}})()');var s=document.createElement('script');s.src='${RESOURCES}';document.body.appendChild(s);Module['preRun'].push(function(){Module['addRunDependency']('${RESOURCES}.loader')});s.onload=function(){if (Module.finishedDataFileDownloads < Module.expectedDataFileDownloads) setTimeout(s.onload, 100); else Module['removeRunDependency']('${RESOURCES}.loader')};")
    target_link_libraries(${TARGET} PRIVATE "--pre-js ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCES}.load.js")
    add_dependencies(${TARGET} ${RESOURCES})
endfunction ()
