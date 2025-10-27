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
include_guard(DIRECTORY)

include(GNUInstallDirs)
include(${CMAKE_CURRENT_LIST_DIR}/ucm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/VSSolution.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/UrhoOptions.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CCache.cmake)

get_cmake_property(MULTI_CONFIG_PROJECT GENERATOR_IS_MULTI_CONFIG)

# Prevent use of undefined build type, default to Debug. Done here instead of later so that URHO3D_CONFIG
# is properly set. Also normalize the case of CMAKE_BUILD_TYPE to match CMAKE_CONFIGURATION_TYPES.
if (NOT MULTI_CONFIG_PROJECT)
    if (NOT CMAKE_BUILD_TYPE)
        string(TOUPPER "${CMAKE_CONFIGURATION_TYPES}" CONFIG_TYPES_UPPER)
        string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
        list(FIND CONFIG_TYPES_UPPER "${BUILD_TYPE_UPPER}" CONFIG_INDEX)
        if (CONFIG_INDEX GREATER_EQUAL 0)
            list(GET CMAKE_CONFIGURATION_TYPES ${CONFIG_INDEX} NORMALIZED_BUILD_TYPE)
        endif()
    endif ()

    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Specifies the build type." FORCE)
    endif ()

    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
endif ()

if (MULTI_CONFIG_PROJECT)
    set (URHO3D_CONFIG $<CONFIG>)
else ()
    set (URHO3D_CONFIG ${CMAKE_BUILD_TYPE})
endif ()

set(CMAKE_INSTALL_BINDIR_BASE ${CMAKE_INSTALL_BINDIR})
set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR}/${URHO3D_CONFIG})
if (ANDROID)
    set (CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_BINDIR_BASE}/${URHO3D_CONFIG})
else ()
    set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}/${URHO3D_CONFIG})
endif ()

if (Urho3D_IS_SDK)
    set (URHO3D_SWIG_LIB_DIR ${Urho3D_PACKAGE_ROOT}/include/swig/Lib)
    set (URHO3D_CMAKE_DIR ${Urho3D_PACKAGE_ROOT}/share/CMake)
    set (URHO3D_CSHARP_PROPS_FILE ${URHO3D_CMAKE_DIR}/Directory.Build.props)
    set (URHO3D_TEMPLATE_DIR ${Urho3D_PACKAGE_ROOT}/include)
else ()
    set (URHO3D_SWIG_LIB_DIR ${rbfx_SOURCE_DIR}/Source/ThirdParty/swig/Lib)
    set (URHO3D_CMAKE_DIR ${rbfx_SOURCE_DIR}/CMake)
    set (URHO3D_CSHARP_PROPS_FILE ${rbfx_SOURCE_DIR}/Directory.Build.props)
    set (URHO3D_TEMPLATE_DIR ${rbfx_SOURCE_DIR}/Source/Urho3D)
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

if (NOT DESKTOP)
    find_package(Urho3DTools QUIET NO_CMAKE_INSTALL_PREFIX)
    if (URHO3D_PACKAGING AND (NOT Urho3DTools_FOUND OR NOT TARGET PackageTool))
        message(FATAL_ERROR "PackageTool not found, please provide Urho3DTools in CMAKE_PREFIX_PATH")
    endif ()

    if (URHO3D_CSHARP AND (NOT Urho3DTools_FOUND OR NOT TARGET swig))
        message(FATAL_ERROR "swig not found, please provide Urho3DTools in CMAKE_PREFIX_PATH")
    endif ()

    if (Urho3DTools_FOUND)
        message(STATUS "Found Urho3DTools: ${Urho3DTools_VERSION}")
    endif ()
endif ()

# Note: We use $<TARGET_FILE:swig> and $<TARGET_FILE:PackageTool> directly in commands
# instead of setting SWIG_EXECUTABLE and PACKAGE_TOOL variables

# Xcode does not support per-config source files, therefore we must lock generated bindings to some config
# and they wont switch when build config changes in Xcode.
if (CMAKE_GENERATOR STREQUAL "Xcode")
    if (CMAKE_BUILD_TYPE)
        set (URHO3D_CSHARP_BIND_CONFIG "${CMAKE_BUILD_TYPE}")
    else ()
        set (URHO3D_CSHARP_BIND_CONFIG "Release")
    endif ()
elseif (MULTI_CONFIG_PROJECT)
    set (URHO3D_CSHARP_BIND_CONFIG $<CONFIG>)
elseif (CMAKE_BUILD_TYPE)
    set (URHO3D_CSHARP_BIND_CONFIG ${CMAKE_BUILD_TYPE})
else ()
    set (URHO3D_CSHARP_BIND_CONFIG Release)
endif ()

if (EMSCRIPTEN)
    set (EMPACKAGER python ${EMSCRIPTEN_ROOT_PATH}/tools/file_packager.py CACHE PATH "file_packager.py")
    set (EMCC_WITH_SOURCE_MAPS_FLAG -gsource-map --source-map-base=. -fdebug-compilation-dir='.' -gseparate-dwarf)
endif ()

# Generate CMake.props which will be included in .csproj files. This function should be called after all targets are
# added to the project, because it depends on target properties.
function (rbfx_configure_cmake_props)
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
        URHO3D_CSHARP
        URHO3D_CSHARP_PROPS_FILE
        URHO3D_PLATFORM
        Urho3D_IS_SDK
        Urho3D_PACKAGE_ROOT
        URHO3D_NETFX
        URHO3D_NETFX_RUNTIME_IDENTIFIER
        URHO3D_NETFX_RUNTIME)
        string(REPLACE "$<CONFIG>" "$(Configuration)" var_value "${${var}}")
        file(APPEND "${PROPS_OUT}" "    <${var}>${var_value}</${var}>\n")
    endforeach ()

    # Binary/sourece dirs
    get_cmake_property(vars VARIABLES)
    list (SORT vars)
    foreach (var ${vars})
        if ("${var}" MATCHES "_(BINARY|SOURCE)_DIR$")
            if (NOT "${${var}}" MATCHES "^.+/ThirdParty/.+$")
                string(REPLACE "." "_" var_name "${var}")
                set(var_value "${${var}}")
                string(REPLACE "$<CONFIG>" "$(Configuration)" var_value "${var_value}")
                if ("${var_value}" MATCHES "/")
                    # Paths end with /
                    if (NOT "${var_value}" MATCHES "/$")
                        set(var_value "${var_value}/")
                    endif ()
                elseif ("${var_value}" MATCHES "1|ON|YES|TRUE|Y|on|yes|true|y|On|Yes|True")
                    set(var_value "ON")
                elseif ("${var_value}" MATCHES "0|OFF|NO|FALSE|Y|off|no|false|n|Off|No|False")
                    set(var_value "OFF")
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
    if (NOT Urho3D_IS_SDK)
        install (FILES ${rbfx_SOURCE_DIR}/Directory.Build.props DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/)
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
    file (GLOB files LIST_DIRECTORIES false ${CMAKE_CURRENT_SOURCE_DIR}/*)
    source_group("" FILES ${files})

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
    install (FILES "${CS_OUTPUT}.runtimeconfig.json" DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
    install (FILES "${CS_OUTPUT}.dll" DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL)
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

    if (NOT UWP)
        # UWP does not support UnmanagedType.CustomMarshaler
        list(APPEND GENERATOR_OPTIONS -DSWIG_CSHARP_NO_STRING_HELPER=1)
    endif ()

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

    # Platform defines with "__" prefix and suffix
    foreach(def IOS WEB ANDROID APPLE)
        if (${${def}})
            list (APPEND GENERATOR_OPTIONS -D__${def}__=1)
        endif ()
    endforeach()

    # Other platform defines
    foreach(def WIN32 MSVC LINUX UNIX MACOS UWP)
        if (${${def}})
            list (APPEND GENERATOR_OPTIONS -D${def}=1)
        endif ()
    endforeach()

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
    file(GENERATE OUTPUT "GeneratorOptions_${BIND_TARGET}_${URHO3D_CSHARP_BIND_CONFIG}.txt" CONTENT "${GENERATOR_OPTIONS}" CONDITION $<COMPILE_LANGUAGE:CXX>)

    # Swig generator command
    add_custom_command(OUTPUT ${BIND_OUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${BIND_OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BIND_OUT_DIR}
        COMMAND "${CMAKE_COMMAND}" -E env "SWIG_LIB=${URHO3D_SWIG_LIB_DIR}" "$<TARGET_FILE:swig>"
        ARGS @"${CMAKE_CURRENT_BINARY_DIR}/GeneratorOptions_${BIND_TARGET}_${URHO3D_CSHARP_BIND_CONFIG}.txt" > ${CMAKE_CURRENT_BINARY_DIR}/swig_${BIND_TARGET}.log

        MAIN_DEPENDENCY ${BIND_SWIG}
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/GeneratorOptions_${BIND_TARGET}_${URHO3D_CSHARP_BIND_CONFIG}.txt" ${BIND_DEPENDS} swig
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "SWIG: Generating C# bindings for ${BIND_TARGET}")

    if (BIND_EMBED)
        # Bindings are part of another target
        target_sources(${BIND_EMBED} PRIVATE ${BIND_OUT_FILE})
    else ()
        # Bindings are part of target bindings are generated for
        target_sources(${BIND_TARGET} PRIVATE ${BIND_OUT_FILE})
    endif ()

    if (BIND_CSPROJ)
        get_filename_component(BIND_MANAGED_TARGET "${BIND_CSPROJ}" NAME_WLE)
        add_target_csharp(
            TARGET ${BIND_MANAGED_TARGET}
            PROJECT ${BIND_CSPROJ}
            OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BIND_MANAGED_TARGET})
        if (TARGET ${BIND_MANAGED_TARGET})
            # Real C# target
            add_dependencies(${BIND_MANAGED_TARGET} ${BIND_TARGET} ${BIND_EMBED})
        endif ()
    endif ()
endfunction ()

function (create_pak PAK_DIR PAK_FILE)
    cmake_parse_arguments(PARSE_ARGV 2 PAK "NO_COMPRESS" "" "DEPENDS")

    get_filename_component(NAME "${PAK_FILE}" NAME)

    if (NOT PAK_NO_COMPRESS)
        list (APPEND PAK_FLAGS -c)
    endif ()

    set_property (SOURCE ${PAK_FILE} PROPERTY GENERATED TRUE)
    add_custom_command(
        OUTPUT "${PAK_FILE}"
        COMMAND "$<TARGET_FILE:PackageTool>" "${PAK_DIR}" "${PAK_FILE}" -q ${PAK_FLAGS}
        DEPENDS PackageTool ${PAK_DEPENDS}
        COMMENT "Packaging ${NAME}"
    )
endfunction ()

function (web_executable TARGET)
    # TARGET target_name                            - A name of target.
    # THREADING                                     - Whether to enable multithreading for this target.
    # Sets up a target for deployment to web.
    # Use this macro on targets that should compile for web platform, possibly right after add_executable().
    cmake_parse_arguments(WEB_EXECUTABLE "THREADING" "" "" ${ARGN})

    if (WEB)
        set_target_properties (${TARGET} PROPERTIES SUFFIX .html)
        target_link_libraries(${TARGET} PRIVATE -sNO_EXIT_RUNTIME=1 -sFORCE_FILESYSTEM=1 -sASSERTIONS=0 -lidbfs.js)
        target_compile_options(${TARGET} PRIVATE -pthread)
        target_link_options(${TARGET} PRIVATE -pthread -sUSE_PTHREADS=1)
        if (BUILD_SHARED_LIBS)
            target_link_libraries(${TARGET} PRIVATE -sMAIN_MODULE=1)
        endif ()
        if (TARGET datachannel-wasm)
            if (Urho3D_IS_SDK)
                set (LIBDATACHANNEL_WASM_DIR "${Urho3D_PACKAGE_ROOT}/include/libdatachannel-wasm")
            else ()
                set (LIBDATACHANNEL_WASM_DIR "${rbfx_SOURCE_DIR}/Source/ThirdParty/libdatachannel-wasm")
            endif ()
            target_link_options(${TARGET} PRIVATE
                "SHELL:--js-library ${LIBDATACHANNEL_WASM_DIR}/wasm/js/webrtc.js"
                "SHELL:--js-library ${LIBDATACHANNEL_WASM_DIR}/wasm/js/websocket.js"
            )
        endif ()
        if (WEB_EXECUTABLE_THREADING)
            target_link_libraries (${TARGET} PRIVATE "-sPTHREAD_POOL_SIZE=\"!!globalThis.SharedArrayBuffer && self.crossOriginIsolated ? navigator.hardwareConcurrency - 1 : 0\"")
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

    configure_file (${URHO3D_TEMPLATE_DIR}/Resources.load.js.in ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCES}.load.js @ONLY)
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

    configure_file (${URHO3D_TEMPLATE_DIR}/LinkedPlugins.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/LinkedPlugins.cpp @ONLY)
    target_sources (${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/LinkedPlugins.cpp)
    target_link_libraries (${TARGET} PRIVATE ${STATIC_PLUGIN_LIBRARIRES})
endfunction()

function (install_third_party_libs)
    if (NOT URHO3D_MERGE_STATIC_LIBS)
        foreach (TARGET ${ARGV})
            if (TARGET ${TARGET})
                install (TARGETS ${TARGET}
                    EXPORT Urho3DThirdParty
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    COMPONENT ThirdParty)
                # Also add to main Urho3D export for backward compatibility
                install (TARGETS ${TARGET}
                    EXPORT Urho3D
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    COMPONENT ThirdParty)
            endif ()
        endforeach ()
    endif ()
endfunction ()

function (install_third_party_tools)
    foreach (TARGET ${ARGV})
        if (TARGET ${TARGET})
            install (TARGETS ${TARGET}
                EXPORT Urho3DThirdParty
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT ThirdParty
                PERMISSIONS ${PERMISSIONS_755})
            # Also add to Urho3DTools export
            install (TARGETS ${TARGET}
                EXPORT Urho3DTools
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Tools
                PERMISSIONS ${PERMISSIONS_755})
            # Also add to main Urho3D export for backward compatibility
            install (TARGETS ${TARGET}
                EXPORT Urho3D
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Tools
                PERMISSIONS ${PERMISSIONS_755})
        endif ()
    endforeach ()
endfunction ()

function (install_tools)
    foreach (TARGET ${ARGV})
        if (TARGET ${TARGET})
            install (TARGETS ${TARGET}
                EXPORT Urho3DTools
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Tools
                PERMISSIONS ${PERMISSIONS_755})
            # Also add to main Urho3D export for backward compatibility
            install (TARGETS ${TARGET}
                EXPORT Urho3D
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                COMPONENT Tools
                PERMISSIONS ${PERMISSIONS_755})
        endif ()
    endforeach ()
endfunction ()

# Install runtime dependencies for targets with component support
function (install_target_runtime_deps)
    if(NOT DESKTOP)
        return()
    endif()

    cmake_parse_arguments(ARG "" "TARGET;COMPONENT" "ADDITIONAL_MODULES" ${ARGN})

    if (NOT ARG_TARGET)
        message(FATAL_ERROR "TARGET must be specified")
    endif()

    # Check if target exists
    if (NOT TARGET ${ARG_TARGET})
        message(WARNING "Target '${ARG_TARGET}' does not exist")
        return()
    endif()

    # Get target type
    get_target_property(target_type ${ARG_TARGET} TYPE)

    # Only process executable and shared library targets
    if (NOT target_type STREQUAL "EXECUTABLE" AND
        NOT target_type STREQUAL "SHARED_LIBRARY" AND
        NOT target_type STREQUAL "MODULE_LIBRARY")
        message(WARNING "Target '${ARG_TARGET}' is not an executable or shared library")
        return()
    endif()

    # Build COMPONENT argument if provided
    set(COMPONENT_ARG "")
    if (ARG_COMPONENT)
        set(COMPONENT_ARG "COMPONENT" "${ARG_COMPONENT}")
    endif()

    # Use install(CODE) with file(GET_RUNTIME_DEPENDENCIES) for CMake 3.21+
    install(CODE "
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES \"$<TARGET_FILE:${ARG_TARGET}>\"
            RESOLVED_DEPENDENCIES_VAR _r_deps
            UNRESOLVED_DEPENDENCIES_VAR _u_deps
            PRE_EXCLUDE_REGEXES \"api-ms-.*\" \"ext-ms-.*\"
            POST_EXCLUDE_REGEXES \".*system32/.*\\\\.dll\" \".*syswow64/.*\\\\.dll\" \"/usr/lib\" \"/lib\"
            DIRECTORIES \"${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}\" ${ARG_ADDITIONAL_MODULES}
        )
        foreach(_file \${_r_deps})
            file(INSTALL
                DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
                TYPE SHARED_LIBRARY
                FILES \"\${_file}\"
            )
        endforeach()
        " ${COMPONENT_ARG}
    )
endfunction()

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

# Function to create component-specific packaging targets
function(create_pack_target COMPONENT_NAME)
    # Create a unique target name (lowercase)
    string(TOLOWER "${COMPONENT_NAME}" COMPONENT_NAME_LOWER)
    set(TARGET_NAME "pack-${COMPONENT_NAME_LOWER}")

    # Set component-specific variables for the template
    set(COMPONENT_NAME_INTERNAL ${COMPONENT_NAME})

    # Determine build type suffix
    if(BUILD_SHARED_LIBS)
        set(BUILD_TYPE_SUFFIX "dll")
    else()
        set(BUILD_TYPE_SUFFIX "lib")
    endif()

    # Configure CPack for this specific component
    configure_file(${rbfx_SOURCE_DIR}/CMake/Modules/CPackComponent.cmake.in
        ${CMAKE_BINARY_DIR}/CPackConfig-${COMPONENT_NAME}.cmake @ONLY)

    # Create the custom target that runs CPack with the component-specific config
    if(MULTI_CONFIG_PROJECT)
        # For multi-config generators (Visual Studio, Xcode), package all configurations
        # We need to create a script that calls CPack with all configurations
        set(PACK_SCRIPT "${CMAKE_BINARY_DIR}/pack-${COMPONENT_NAME}.cmake")
        file(WRITE "${PACK_SCRIPT}"
"# Auto-generated script to package all configurations
set(CONFIGS \"${CMAKE_CONFIGURATION_TYPES}\")
string(REPLACE \";\" \";\" CONFIGS_LIST \"\${CONFIGS}\")
execute_process(
    COMMAND \"${CMAKE_CPACK_COMMAND}\" -G ZIP -C \"\${CONFIGS_LIST}\" --config \"${CMAKE_BINARY_DIR}/CPackConfig-${COMPONENT_NAME}.cmake\"
    WORKING_DIRECTORY \"${CMAKE_BINARY_DIR}\"
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR \"CPack failed with error code \${result}\")
endif()
")

        add_custom_target(${TARGET_NAME}
            COMMAND ${CMAKE_COMMAND} -P ${PACK_SCRIPT}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Creating package for ${COMPONENT_NAME} component (all configurations)"
            VERBATIM
        )
    else()
        # For single-config generators (Make, Ninja)
        add_custom_target(${TARGET_NAME}
            COMMAND ${CMAKE_CPACK_COMMAND} --config ${CMAKE_BINARY_DIR}/CPackConfig-${COMPONENT_NAME}.cmake
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Creating package for ${COMPONENT_NAME} component"
            VERBATIM
        )
    endif()

    # Mark as utility target
    set_target_properties(${TARGET_NAME} PROPERTIES
        EXCLUDE_FROM_ALL ON
        EXCLUDE_FROM_DEFAULT_BUILD ON
        FOLDER "Package")
endfunction()

# Function to create a target that packages all components
function(create_pack_all_target)
    add_custom_target(pack-all
        COMMENT "Creating package for all components"
        DEPENDS pack-sdk pack-tools pack-samples
        VERBATIM
    )

    # Mark as utility target
    set_target_properties(pack-all PROPERTIES
        EXCLUDE_FROM_ALL ON
        EXCLUDE_FROM_DEFAULT_BUILD ON
        FOLDER "Package")
endfunction()
