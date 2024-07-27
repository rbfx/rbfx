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

include(${CMAKE_CURRENT_LIST_DIR}/ucm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/VSSolution.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CCache.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/UrhoOptions.cmake)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/../Urho3D_GeneratedConfig.cmake)
    set (URHO3D_IS_SDK ON)
    set (URHO3D_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../)
    get_filename_component(URHO3D_SDK_PATH "${URHO3D_SDK_PATH}" REALPATH)
else ()
    set (URHO3D_IS_SDK OFF)
endif ()

if (URHO3D_IS_SDK)
    set (URHO3D_THIRDPARTY_DIR ${URHO3D_SDK_PATH}/include/Urho3D/ThirdParty)
    set (URHO3D_CMAKE_DIR ${URHO3D_SDK_PATH}/share/CMake)
    set (URHO3D_CSHARP_PROPS_FILE ${URHO3D_CMAKE_DIR}/Directory.Build.props)
else ()
    set (URHO3D_THIRDPARTY_DIR ${rbfx_SOURCE_DIR}/Source/ThirdParty)
    set (URHO3D_CMAKE_DIR ${rbfx_SOURCE_DIR}/CMake)
    set (URHO3D_CSHARP_PROPS_FILE ${rbfx_SOURCE_DIR}/Directory.Build.props)
endif ()

set(PERMISSIONS_644 OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)
set(PERMISSIONS_755 OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

if (EMSCRIPTEN AND "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    # Workaround for following error:
    #   The install of the Samples target requires changing an RPATH from the build
    #   tree, but this is not supported with the Ninja generator unless on an
    #   ELF-based platform.  The CMAKE_BUILD_WITH_INSTALL_RPATH variable may be set
    #   to avoid this relinking step.
    set (CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif ()

# Ensure variable is in the cache.
set(RBFX_CSPROJ_LIST "" CACHE STRING "A list of C# projects." FORCE)

if (DEFINED URHO3D_SDK)
    if (DESKTOP)
        message(FATAL_ERROR "URHO3D_SDK is can not be defined for a desktop platform.")
    else ()
        include("${URHO3D_SDK}/share/CMake/SDKTools.cmake")
    endif ()
endif ()

if (NOT DEFINED URHO3D_SDK)
    set (PACKAGE_TOOL $<TARGET_FILE:PackageTool>)
    set (SWIG_EXECUTABLE $<TARGET_FILE:swig>)
endif ()

# Xcode does not support per-config source files, therefore we must lock generated bindings to some config
# and they wont switch when build config changes in Xcode.
if (CMAKE_GENERATOR STREQUAL "Xcode")
    if (CMAKE_BUILD_TYPE)
        set (URHO3D_CSHARP_BIND_CONFIG "${CMAKE_BUILD_TYPE}")
    else ()
        set (URHO3D_CSHARP_BIND_CONFIG "Release")
    endif ()
elseif (GENERATOR_IS_MULTI_CONFIG)
    set (URHO3D_CSHARP_BIND_CONFIG $<CONFIG>)
elseif (CMAKE_BUILD_TYPE)
    set (URHO3D_CSHARP_BIND_CONFIG ${CMAKE_BUILD_TYPE})
else ()
    set (URHO3D_CSHARP_BIND_CONFIG Release)
endif ()

if (EMSCRIPTEN)
    set (WEB ON)
    set (EMPACKAGER python ${EMSCRIPTEN_ROOT_PATH}/tools/file_packager.py CACHE PATH "file_packager.py")
    set (EMCC_WITH_SOURCE_MAPS_FLAG -gsource-map --source-map-base=. -fdebug-compilation-dir='.' -gseparate-dwarf)
endif ()

# Prevent use of undefined build type, default to Debug. Done here instead of UrhoOptions.cmake so that user projects
# can take advantage of this behavior as UrhoCommon.cmake will be included earlier, most likely before any targets are
# defined.
if (NOT MULTI_CONFIG_PROJECT AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Specifies the build type." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
endif ()

# Generate CMake.props which will be included in .csproj files. This function should be called after all targets are
# added to the project, because it depends on target properties.
function (rbfx_configure_cmake_props)
    if (NOT URHO3D_CSHARP)
        return ()
    endif ()

    if (NOT PROJECT_IS_TOP_LEVEL)
        return ()
    endif ()

    cmake_parse_arguments(PROPS "" "OUT" "" ${ARGN})

    if (NOT DEFINED PROPS_OUT)
        set(PROPS_OUT "${CMAKE_BINARY_DIR}/CMake.props")
    endif ()

    file(WRITE "${PROPS_OUT}" "<Project>\n")
    file(APPEND "${PROPS_OUT}" "  <PropertyGroup>\n")
    file(APPEND "${PROPS_OUT}" "    <CMakePropsIncluded>true</CMakePropsIncluded>\n")

    # Variables of interest
    foreach (var
        CMAKE_GENERATOR
        CMAKE_RUNTIME_OUTPUT_DIRECTORY
        CMAKE_CONFIGURATION_TYPES
        URHO3D_CSHARP_PROPS_FILE
        URHO3D_PLATFORM
        URHO3D_IS_SDK
        URHO3D_SDK_PATH
        URHO3D_NETFX
        URHO3D_NETFX_RUNTIME_IDENTIFIER
        URHO3D_NETFX_RUNTIME)
        file(APPEND "${PROPS_OUT}" "    <${var}>${${var}}</${var}>\n")
    endforeach ()

    # Binary/sourece dirs
    get_cmake_property(vars VARIABLES)
    list (SORT vars)
    foreach (var ${vars})
        if ("${var}" MATCHES "_(BINARY|SOURCE)_DIR$")
            if (NOT "${${var}}" MATCHES "^.+/ThirdParty/.+$")
                string(REPLACE "." "_" var_name "${var}")
                set(var_value "${${var}}")
                if (NOT "${var_value}" MATCHES "/$")
                    set(var_value "${var_value}/")
                endif ()
                file(APPEND "${PROPS_OUT}" "    <${var_name}>${var_value}</${var_name}>\n")
            endif ()
        endif ()
    endforeach()

    # C# build config, translate CMake generator expression to MSBuild expression.
    string(REPLACE "$<CONFIG>" "$(Configuration)" URHO3D_CSHARP_BIND_CONFIG_PROP "${URHO3D_CSHARP_BIND_CONFIG}")
    file(APPEND "${PROPS_OUT}" "    <URHO3D_CSHARP_BIND_CONFIG>${URHO3D_CSHARP_BIND_CONFIG_PROP}</URHO3D_CSHARP_BIND_CONFIG>\n")

    file(APPEND "${PROPS_OUT}" "  </PropertyGroup>\n")
    file(APPEND "${PROPS_OUT}" "</Project>\n")
endfunction ()

if (URHO3D_CSHARP)
    find_program(DOTNET dotnet)
    if (NOT DOTNET)
        message(FATAL_ERROR "dotnet executable was not found.")
    endif ()

    # Detect runtime version
    execute_process(COMMAND ${DOTNET} --list-sdks OUTPUT_VARIABLE DOTNET_SDKS OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "\\.[0-9]+ [^\r\n]+" "" DOTNET_SDKS "${DOTNET_SDKS}")      # Trim patch version and SDK paths
    string(REGEX REPLACE "\r?\n" ";" DOTNET_SDKS "${DOTNET_SDKS}")                  # Turn lines into a list
    list(REVERSE DOTNET_SDKS)                                                       # Highest version goes first
    if (URHO3D_NETFX_RUNTIME_VERSION)
        string (REPLACE "." "\\." URHO3D_NETFX_RUNTIME_VERSION "${URHO3D_NETFX_RUNTIME_VERSION}")
        string (REPLACE "*" "." URHO3D_NETFX_RUNTIME_VERSION "${URHO3D_NETFX_RUNTIME_VERSION}")
        set (URHO3D_NETFX_RUNTIME_VERSION_REGEX "${URHO3D_NETFX_RUNTIME_VERSION}")
        unset (URHO3D_NETFX_RUNTIME_VERSION)
        foreach (VERSION ${DOTNET_SDKS})
            if ("${VERSION}" MATCHES "${URHO3D_NETFX_RUNTIME_VERSION_REGEX}")
                set (URHO3D_NETFX_RUNTIME_VERSION "${VERSION}")
                break ()
            endif ()
        endforeach ()
    endif ()
    if (NOT URHO3D_NETFX_RUNTIME_VERSION)
        list (GET DOTNET_SDKS 0 URHO3D_NETFX_RUNTIME_VERSION)
    endif ()

    # Trim extra elements that may be preset in version string. Eg. "6.0.100-preview.6.21355.2" is converted to "6.0".
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)(.+)?" "\\1.\\2" URHO3D_NETFX_RUNTIME_VERSION "${URHO3D_NETFX_RUNTIME_VERSION}")
    if (URHO3D_NETFX_RUNTIME_VERSION VERSION_LESS "5.0")
        set (NETCORE core)
    endif ()
    set (URHO3D_NETFX_RUNTIME "net${NETCORE}${URHO3D_NETFX_RUNTIME_VERSION}")

    if (WIN32)
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER win-${URHO3D_PLATFORM})
    elseif (MACOS)
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER osx-${URHO3D_PLATFORM})
    else ()
        set (URHO3D_NETFX_RUNTIME_IDENTIFIER linux-${URHO3D_PLATFORM})
    endif ()

    set (DOTNET_FRAMEWORK_TYPES net6 net7)
    set (DOTNET_FRAMEWORK_VERSIONS v6.0 v7.0)
    list (FIND DOTNET_FRAMEWORK_TYPES ${URHO3D_NETFX} DOTNET_FRAMEWORK_INDEX)
    if (DOTNET_FRAMEWORK_INDEX GREATER -1)
        list (GET DOTNET_FRAMEWORK_VERSIONS ${DOTNET_FRAMEWORK_INDEX} CMAKE_DOTNET_TARGET_FRAMEWORK_VERSION)
    endif ()
    unset (DOTNET_FRAMEWORK_INDEX)

    # For .csproj embedded into visual studio solution
    if (NOT URHO3D_IS_SDK)
        install (FILES ${rbfx_SOURCE_DIR}/Directory.Build.props DESTINATION ${DEST_SHARE_DIR}/CMake/)
    endif ()
endif()

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
    if (CMAKE_HOST_WIN32 OR CMAKE_HOST_SYSTEM MATCHES "^Windows-")
        if (IS_DIRECTORY ${ABS_SOURCE})
            set (SLASH_J /J)
        else ()
            unset (SLASH_J)
        endif ()
        set (RESULT_CODE 1)
        if(${CMAKE_HOST_SYSTEM_VERSION} GREATER_EQUAL 6.0.0)
            if (NOT EXISTS ${ABS_DESTINATION})
                # Have to use string-REPLACE as file-TO_NATIVE_PATH does not work as expected with MinGW on "backward slash" host system
                string (REPLACE / \\ BACKWARD_ABS_DESTINATION ${ABS_DESTINATION})
                string (REPLACE / \\ BACKWARD_ABS_SOURCE ${ABS_SOURCE})
                execute_process (COMMAND cmd /C mklink ${SLASH_J} ${BACKWARD_ABS_DESTINATION} ${BACKWARD_ABS_SOURCE} OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE RESULT_CODE)
            endif ()
        endif ()
        if (NOT "${RESULT_CODE}" STREQUAL "0")
            if (SLASH_J)
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
function (group_sources)
    file (GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/**)
    foreach (child ${children})
        if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
            string(REPLACE "/" "\\" groupname "${child}")
            file (GLOB files LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/${child}/*)
            source_group(${groupname} FILES ${files})
        endif ()
    endforeach ()
endfunction ()

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
        list(APPEND RBFX_CSPROJ_LIST ${CS_PROJECT})
        set(RBFX_CSPROJ_LIST "${RBFX_CSPROJ_LIST}" CACHE STRING "A list of C# projects." FORCE)
    endif ()
    install (FILES "${CS_OUTPUT}.runtimeconfig.json" DESTINATION "${DEST_BIN_DIR_CONFIG}" OPTIONAL)
    install (FILES "${CS_OUTPUT}.dll" DESTINATION "${DEST_BIN_DIR_CONFIG}" OPTIONAL)
endfunction ()

function (csharp_bind_target)
    if (NOT URHO3D_CSHARP)
        return ()
    endif ()

    cmake_parse_arguments(BIND "" "TARGET;CSPROJ;SWIG;NAMESPACE;EMBED;DEPENDS" "" ${ARGN})

    # General SWIG parameters
    set(BIND_OUT_DIR  ${CMAKE_CURRENT_BINARY_DIR}/${BIND_TARGET}CSharp_${URHO3D_CSHARP_BIND_CONFIG})
    set(BIND_OUT_FILE ${BIND_OUT_DIR}/${BIND_TARGET}CSHARP_wrap.cxx)
    set(GENERATOR_OPTIONS
        -csharp
        -namespace "${BIND_NAMESPACE}"
        -fastdispatch
        -c++
        -outdir "${BIND_OUT_DIR}"
        -o "${BIND_OUT_FILE}"
    )

    if (URHO3D_SWIG_DEBUG_TMSEARCH)
        list(APPEND GENERATOR_OPTIONS -debug-tmsearch)
    endif ()

    # Native library name matches target name by default
    if (BIND_EMBED)
        list (APPEND GENERATOR_OPTIONS -dllimport $<TARGET_NAME:${BIND_EMBED}>)
    else ()
        if (IOS OR WEB)
            list (APPEND GENERATOR_OPTIONS -dllimport __Internal)
        else ()
            list (APPEND GENERATOR_OPTIONS -dllimport $<TARGET_NAME:${BIND_TARGET}>)
        endif ()
    endif ()

    if (IOS)
        list (APPEND GENERATOR_OPTIONS -D__IOS__)
    endif ()

    # Parse bindings using same compile definitions as built target
    __TARGET_GET_PROPERTIES_RECURSIVE(INCLUDES ${BIND_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    __TARGET_GET_PROPERTIES_RECURSIVE(DEFINES  ${BIND_TARGET} INTERFACE_COMPILE_DEFINITIONS)
    if (TARGET "${BIND_EMBED}")
        __TARGET_GET_PROPERTIES_RECURSIVE(INCLUDES ${BIND_EMBED} INTERFACE_INCLUDE_DIRECTORIES)
        __TARGET_GET_PROPERTIES_RECURSIVE(DEFINES  ${BIND_EMBED} INTERFACE_COMPILE_DEFINITIONS)
    endif ()

    if (BIND_DEPENDS)
        foreach(dep ${BIND_DEPENDS})
            __TARGET_GET_PROPERTIES_RECURSIVE(INCLUDES ${dep} INTERFACE_INCLUDE_DIRECTORIES)
            __TARGET_GET_PROPERTIES_RECURSIVE(DEFINES  ${dep} INTERFACE_COMPILE_DEFINITIONS)
        endforeach()
    endif ()


    if (INCLUDES)
        list (REMOVE_DUPLICATES INCLUDES)
    endif ()
    if (DEFINES)
        list (REMOVE_DUPLICATES DEFINES)
    endif ()
    foreach(item ${INCLUDES})
        list(APPEND GENERATOR_OPTIONS -I${item})
    endforeach()
    # Defines must have a value, otherwise SWIG gets confused
    foreach(item ${DEFINES})
        string(FIND "${item}" "=" EQUALITY_INDEX)
        if (EQUALITY_INDEX EQUAL -1)
            set (item "${item}=1")
        endif ()
        list(APPEND GENERATOR_OPTIONS "-D${item}")
    endforeach()

    # Main .i file
    list(APPEND GENERATOR_OPTIONS ${BIND_SWIG})

    # Finalize option list
    string(REGEX REPLACE "[^;]+\\$<COMPILE_LANGUAGE:[^;]+;" "" GENERATOR_OPTIONS "${GENERATOR_OPTIONS}")    # COMPILE_LANGUAGE creates ambiguity, remove.
    list(REMOVE_DUPLICATES GENERATOR_OPTIONS)
    string(REPLACE ";" "\n" GENERATOR_OPTIONS "${GENERATOR_OPTIONS}")
    file(GENERATE OUTPUT "GeneratorOptions_${BIND_TARGET}_$<CONFIG>.txt" CONTENT "${GENERATOR_OPTIONS}" CONDITION $<COMPILE_LANGUAGE:CXX>)

    # Swig generator command
    add_custom_command(OUTPUT ${BIND_OUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${BIND_OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BIND_OUT_DIR}
        COMMAND "${CMAKE_COMMAND}" -E env "SWIG_LIB=${URHO3D_THIRDPARTY_DIR}/swig/Lib" "${SWIG_EXECUTABLE}"
        ARGS @"${CMAKE_CURRENT_BINARY_DIR}/GeneratorOptions_${BIND_TARGET}_${URHO3D_CSHARP_BIND_CONFIG}.txt" > ${CMAKE_CURRENT_BINARY_DIR}/swig_${BIND_TARGET}.log

        MAIN_DEPENDENCY ${BIND_SWIG}
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/GeneratorOptions_${BIND_TARGET}_${URHO3D_CSHARP_BIND_CONFIG}.txt" ${BIND_DEPENDS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "SWIG: Generating C# bindings for ${BIND_TARGET}")

    if (BIND_EMBED)
        # Bindings are part of another target
        target_sources(${BIND_EMBED} PRIVATE ${BIND_OUT_FILE})
    else ()
        # Bindings are part of target bindings are generated for
        target_sources(${BIND_TARGET} PRIVATE ${BIND_OUT_FILE})
    endif ()

    if (MULTI_CONFIG_PROJECT)
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    else ()
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        # Needed for mono on unixes but not on windows.
        set (FACADES Facades/)
    endif ()
    if (BIND_CSPROJ)
        get_filename_component(BIND_MANAGED_TARGET "${BIND_CSPROJ}" NAME_WLE)
        add_target_csharp(
            TARGET ${BIND_MANAGED_TARGET}
            PROJECT ${BIND_CSPROJ}
            OUTPUT ${NET_OUTPUT_DIRECTORY}/${BIND_MANAGED_TARGET})
        if (TARGET ${BIND_MANAGED_TARGET})
            # Real C# target
            add_dependencies(${BIND_MANAGED_TARGET} ${BIND_TARGET} ${BIND_EMBED})
        endif ()
        install (FILES ${NET_OUTPUT_DIRECTORY}/${BIND_MANAGED_TARGET}.dll DESTINATION ${DEST_LIBRARY_DIR_CONFIG})
    endif ()
endfunction ()

function (create_pak PAK_DIR PAK_FILE)
    cmake_parse_arguments(PARSE_ARGV 2 PAK "NO_COMPRESS" "" "DEPENDS")

    get_filename_component(NAME "${PAK_FILE}" NAME)

    if (NOT PAK_NO_COMPRESS)
        list (APPEND PAK_FLAGS -c)
    endif ()

    if (NOT EXISTS "${PACKAGE_TOOL}")
        if (TARGET PackageTool)
            set(PACKAGE_TOOL "$<TARGET_FILE:PackageTool>")
            set(PACKAGE_TOOL_TARGET PackageTool)
        else ()
            message(FATAL_ERROR "PackageTool is required, but missing. Either set URHO3D_SDK to path of installed SDK built on current host or set PACKAGE_TOOL to path of PackageTool executable.")
        endif ()
    endif ()

    set_property (SOURCE ${PAK_FILE} PROPERTY GENERATED TRUE)
    add_custom_command(
        OUTPUT "${PAK_FILE}"
        COMMAND "${PACKAGE_TOOL}" "${PAK_DIR}" "${PAK_FILE}" -q ${PAK_FLAGS}
        DEPENDS ${PACKAGE_TOOL_TARGET} ${PAK_DEPENDS}
        COMMENT "Packaging ${NAME}"
    )
endfunction ()

function (web_executable TARGET)
    # TARGET target_name                            - A name of target.
    # Sets up a target for deployment to web.
    # Use this macro on targets that should compile for web platform, possibly right after add_executable().
    if (WEB)
        set_target_properties (${TARGET} PROPERTIES SUFFIX .html)
        target_link_libraries(${TARGET} PRIVATE -sNO_EXIT_RUNTIME=1 -sFORCE_FILESYSTEM=1 -sASSERTIONS=0 -lidbfs.js)
        if (BUILD_SHARED_LIBS)
            target_link_libraries(${TARGET} PRIVATE -sMAIN_MODULE=1)
        endif ()
        if (TARGET datachannel-wasm)
            if (URHO3D_IS_SDK)
                set (LIBDATACHANNEL_WASM_DIR "${URHO3D_SDK_PATH}/include/Urho3D/ThirdParty/libdatachannel-wasm")
            else ()
                set (LIBDATACHANNEL_WASM_DIR "${rbfx_SOURCE_DIR}/Source/ThirdParty/libdatachannel-wasm")
            endif ()
            target_link_options(${TARGET} PRIVATE
                "SHELL:--js-library ${LIBDATACHANNEL_WASM_DIR}/wasm/js/webrtc.js"
                "SHELL:--js-library ${LIBDATACHANNEL_WASM_DIR}/wasm/js/websocket.js"
            )
        endif ()
    endif ()
endfunction ()

function (package_resources_web)
    # Packages one or more binary resources for web deployment.
    # FILES [file_path [file_path [...]]]           - One or more files to package into .js file.
    # RELATIVE_DIR dir_path                         - Path relative to which binary files will be accessed.
    # OUTPUT       file_name                        - JS file name that will contain resources.
    # INSTALL_TO   dir_path                         - Destination of packaged files to be installed to.
    if (NOT WEB)
        return ()
    endif ()

    cmake_parse_arguments(PAK "" "RELATIVE_DIR;OUTPUT;INSTALL_TO" "FILES" ${ARGN})
    if (NOT "${PAK_RELATIVE_DIR}" MATCHES "/$")
        set (PAK_RELATIVE_DIR "${PAK_RELATIVE_DIR}/")
    endif ()
    string(LENGTH "${PAK_RELATIVE_DIR}" PAK_RELATIVE_DIR_LEN)

    foreach (file ${PAK_FILES})
        set_property(SOURCE ${file} PROPERTY GENERATED ON)
        string(SUBSTRING "${file}" ${PAK_RELATIVE_DIR_LEN} 999999 rel_file)
        set (PRELOAD_FILES ${PRELOAD_FILES} ${file}@${rel_file})
    endforeach ()

    get_filename_component(LOADER_DIR "${PAK_OUTPUT}" DIRECTORY)
    add_custom_target("${PAK_OUTPUT}"
        COMMAND ${EMPACKAGER} ${PAK_RELATIVE_DIR}${PAK_OUTPUT}.data --preload ${PRELOAD_FILES} --js-output=${PAK_RELATIVE_DIR}${PAK_OUTPUT} --use-preload-cache ${SEPARATE_METADATA}
        SOURCES ${PAK_FILES}
        DEPENDS ${PAK_FILES}
        COMMENT "Packaging ${PAK_OUTPUT}"
    )

    if (TARGET PackageTool)
        add_dependencies("${PAK_OUTPUT}" PackageTool)
    endif ()

    if (PAK_INSTALL_TO)
        install(FILES "${PAK_RELATIVE_DIR}${PAK_OUTPUT}" "${PAK_RELATIVE_DIR}${PAK_OUTPUT}.data" DESTINATION ${PAK_INSTALL_TO})
    endif ()
endfunction ()

function (web_link_resources TARGET RESOURCES)
    # Link packaged resources to a web target.
    # TARGET target_name                                - A name of CMake target.
    # RESOURCES                                         - JS file name to link to.
    if (NOT WEB)
        return ()
    endif ()
    file (WRITE "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCES}.load.js" "var Module;if(typeof Module==='undefined')Module=eval('(function(){try{return Module||{}}catch(e){return{}}})()');var s=document.createElement('script');s.src='${RESOURCES}';document.body.appendChild(s);Module['preRun'].push(function(){Module['addRunDependency']('${RESOURCES}.loader')});s.onload=function(){if (Module.finishedDataFileDownloads < Module.expectedDataFileDownloads) setTimeout(s.onload, 100); else Module['removeRunDependency']('${RESOURCES}.loader')};")
    target_link_libraries(${TARGET} PRIVATE "--pre-js ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCES}.load.js")
    add_dependencies(${TARGET} ${RESOURCES})
endfunction ()

function (setup_plugin_target TARGET PLUGIN_NAME)
    string (REPLACE "." "_" PLUGIN_NAME_SANITATED ${PLUGIN_NAME})

    set_target_properties (${TARGET} PROPERTIES OUTPUT_NAME ${PLUGIN_NAME})
    target_compile_definitions (${TARGET} PRIVATE
        URHO3D_CURRENT_PLUGIN_NAME=${PLUGIN_NAME}
        URHO3D_CURRENT_PLUGIN_NAME_SANITATED=${PLUGIN_NAME_SANITATED}
        ${PLUGIN_NAME_SANITATED}_EXPORT=1
    )
endfunction ()

function (add_plugin TARGET SOURCE_FILES)
    add_library (${TARGET} ${SOURCE_FILES})
    target_link_libraries (${TARGET} PUBLIC Urho3D)
    setup_plugin_target (${TARGET} "${TARGET}")
endfunction ()

function (target_link_plugins TARGET PLUGIN_LIBRARIES)
    set (DECLARE_FUNCTIONS "")
    set (REGISTER_PLUGINS "")
    set (PLUGIN_LIST "")
    set (STATIC_PLUGIN_LIBRARIRES "")
    foreach (PLUGIN_LIBRARY ${PLUGIN_LIBRARIES})
        get_target_property (PLUGIN_NAME ${PLUGIN_LIBRARY} OUTPUT_NAME)
        get_target_property (TARGET_TYPE ${PLUGIN_LIBRARY} TYPE)
        if (TARGET_TYPE STREQUAL STATIC_LIBRARY)
            string (REPLACE "." "_" PLUGIN_NAME_SANITATED ${PLUGIN_NAME})
            string (APPEND DECLARE_FUNCTIONS "    void RegisterPlugin_${PLUGIN_NAME_SANITATED}();\n")
            string (APPEND REGISTER_PLUGINS "    RegisterPlugin_${PLUGIN_NAME_SANITATED}();\n")
            string (APPEND STATIC_PLUGIN_LIBRARIRES "${PLUGIN_LIBRARY};")
        endif ()
        string (APPEND PLUGIN_LIST "${PLUGIN_NAME};")
    endforeach ()

    if (URHO3D_IS_SDK)
        set (TEMPLATE_DIR ${URHO3D_SDK_PATH}/include/Urho3D)
    else ()
        set (TEMPLATE_DIR ${rbfx_SOURCE_DIR}/Source/Urho3D)
    endif ()

    configure_file (${TEMPLATE_DIR}/LinkedPlugins.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/LinkedPlugins.cpp @ONLY)
    target_sources (${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/LinkedPlugins.cpp)
    target_link_libraries (${TARGET} PRIVATE ${STATIC_PLUGIN_LIBRARIRES})
endfunction()

function (install_third_party_libs)
    if (NOT URHO3D_MERGE_STATIC_LIBS)
        foreach (TARGET ${ARGV})
            if (TARGET ${TARGET})
                install (TARGETS ${TARGET} EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
            endif ()
        endforeach ()
    endif ()
endfunction ()

macro (return_if_not_tool ToolName)
    string(TOUPPER "${ToolName}" _TOOL_NAME)
    string(TOUPPER "${URHO3D_TOOLS}" _URHO3D_TOOLS)
    if (NOT DESKTOP)
        unset(_URHO3D_TOOLS)
        # message(STATUS "Tool ${ToolName} OFF (not desktop)")
        return ()
    elseif ("${_URHO3D_TOOLS}" MATCHES "1|ON|YES|TRUE|Y|on|yes|true|y|On|Yes|True")
        # message(STATUS "Tool ${ToolName} ON (all tools enabled)")
    elseif ("${_TOOL_NAME}" IN_LIST _URHO3D_TOOLS)
        # message(STATUS "Tool ${ToolName} ON (specific tools enabled)")
    else ()
        unset(_URHO3D_TOOLS)
        # message(STATUS "Tool ${ToolName} OFF")
        return ()
    endif ()
    unset(_URHO3D_TOOLS)
endmacro ()

macro (assign_bool VARIABLE)
     if (${ARGN})
         set(${VARIABLE} ON CACHE BOOL "" FORCE)
     else ()
         set(${VARIABLE} OFF CACHE BOOL "" FORCE)
     endif ()
endmacro ()

function (set_property_recursive dir property value)
    get_property(targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach (target IN LISTS targets)
        set_target_properties("${target}" PROPERTIES "${property}" "${value}")
    endforeach ()

    get_property(subdirectories DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
    foreach (subdir IN LISTS subdirectories)
        set_property_recursive("${subdir}" "${property}" "${value}")
    endforeach ()
endfunction ()
