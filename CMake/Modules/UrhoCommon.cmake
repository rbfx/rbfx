# Copyright (c) 2008-2020 the Urho3D project.
# Copyright (c) 2020-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

include_guard(DIRECTORY)

include(GNUInstallDirs)
include(${CMAKE_CURRENT_LIST_DIR}/ucm.cmake)
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
    set (URHO3D_TEMPLATE_DIR ${Urho3D_PACKAGE_ROOT}/include)
else ()
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

if (NOT DESKTOP AND URHO3D_PACKAGING)
    find_package(Urho3DTools QUIET NO_CMAKE_INSTALL_PREFIX)
    if (Urho3DTools_FOUND)
        message(STATUS "Found Urho3DTools: ${Urho3DTools_VERSION}")
    endif ()
endif ()

if (EMSCRIPTEN)
    set (EMPACKAGER python ${EMSCRIPTEN_ROOT_PATH}/tools/file_packager.py CACHE PATH "file_packager.py")
    set (EMCC_WITH_SOURCE_MAPS_FLAG -gsource-map --source-map-base=. -fdebug-compilation-dir='.' -gseparate-dwarf)
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

function (setup_external_tool TARGET VARIABLE_NAME)
    cmake_parse_arguments(ARG "QUIET" "" "" ${ARGN})
    if (TARGET ${TARGET})
        set(${VARIABLE_NAME} "$<TARGET_FILE:${TARGET}>" PARENT_SCOPE)
    else ()
        if (NOT DEFINED ${VARIABLE_NAME})
            set(${VARIABLE_NAME} $ENV{${VARIABLE_NAME}})
        endif ()
        if (NOT DEFINED ${VARIABLE_NAME})
            if (NOT ARG_QUIET)
                message(FATAL_ERROR "'${TARGET}' not found and variable '${VARIABLE_NAME}' is not defined.")
            endif ()
            return()
        endif ()
        foreach(item IN LISTS ${VARIABLE_NAME})
            if (NOT EXISTS "${item}")
                if (NOT ARG_QUIET)
                    message(FATAL_ERROR "Variable '${VARIABLE_NAME}' contains '${item}', but the file does not exist.")
                endif ()
                return()
            endif ()
        endforeach()
        set(${VARIABLE_NAME} "${${VARIABLE_NAME}}" PARENT_SCOPE)
    endif ()
endfunction ()

function (create_pak PAK_DIR PAK_FILE)
    cmake_parse_arguments(PARSE_ARGV 2 PAK "NO_COMPRESS" "" "DEPENDS")

    get_filename_component(NAME "${PAK_FILE}" NAME)

    if (NOT PAK_NO_COMPRESS)
        list (APPEND PAK_FLAGS -c)
    endif ()

    # Discover PackageTool once and cache the result
    if (NOT DEFINED PACKAGE_TOOL_EXECUTABLE)
        setup_external_tool(PackageTool PACKAGE_TOOL_EXECUTABLE QUIET)
        if (NOT PACKAGE_TOOL_EXECUTABLE)
            find_package(Python3 COMPONENTS Interpreter QUIET)
            if (Urho3D_IS_SDK)
                set(PY_SCRIPT "${Urho3D_PACKAGE_ROOT}/bin/PackageTool.py")
            else ()
                set(PY_SCRIPT "${rbfx_SOURCE_DIR}/Source/Tools/PackageTool/PackageTool.py")
            endif ()
            if (Python3_FOUND AND EXISTS "${PY_SCRIPT}")
                message(STATUS "PackageTool: using Python fallback (${Python3_EXECUTABLE})")
                set(PACKAGE_TOOL_EXECUTABLE "${Python3_EXECUTABLE};${PY_SCRIPT}")
            else ()
                message(FATAL_ERROR "PackageTool not found. Build tools natively, "
                    "set PACKAGE_TOOL_EXECUTABLE, or install Python3 "
                    "(script: ${PY_SCRIPT}).")
            endif ()
        endif ()
        set(PACKAGE_TOOL_EXECUTABLE "${PACKAGE_TOOL_EXECUTABLE}" CACHE INTERNAL "PackageTool executable path")
    endif ()

    set_property (SOURCE ${PAK_FILE} PROPERTY GENERATED TRUE)
    add_custom_command(
        OUTPUT "${PAK_FILE}"
        COMMAND ${PACKAGE_TOOL_EXECUTABLE} "${PAK_DIR}" "${PAK_FILE}" -q ${PAK_FLAGS}
        DEPENDS $<TARGET_NAME_IF_EXISTS:PackageTool> ${PAK_DEPENDS}
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
        set (LIBDATACHANNEL_WASM_DIR "")
        if (URHO3D_NETWORK)
            if (EXISTS "${Urho3D_PACKAGE_ROOT}/include/libdatachannel-wasm/wasm/js/webrtc.js")
                set (LIBDATACHANNEL_WASM_DIR "${Urho3D_PACKAGE_ROOT}/include/libdatachannel-wasm")
            elseif (DEFINED rbfx_SOURCE_DIR AND EXISTS "${rbfx_SOURCE_DIR}/Source/ThirdParty/libdatachannel-wasm/wasm/js/webrtc.js")
                set (LIBDATACHANNEL_WASM_DIR "${rbfx_SOURCE_DIR}/Source/ThirdParty/libdatachannel-wasm")
            endif ()
        endif ()
        if (LIBDATACHANNEL_WASM_DIR)
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
        DEPENDS ${PAK_FILES} $<TARGET_NAME_IF_EXISTS:PackageTool>
        COMMENT "Packaging ${PAK_OUTPUT}"
    )

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
    string (REPLACE "." "_" PLUGIN_NAME_SANITIZED ${PLUGIN_NAME})

    set_target_properties (${TARGET} PROPERTIES OUTPUT_NAME ${PLUGIN_NAME})
    target_compile_definitions (${TARGET} PRIVATE
        URHO3D_CURRENT_PLUGIN_NAME=${PLUGIN_NAME}
        URHO3D_CURRENT_PLUGIN_NAME_SANITIZED=${PLUGIN_NAME_SANITIZED}
        ${PLUGIN_NAME_SANITIZED}_EXPORT=1
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
            string (REPLACE "." "_" PLUGIN_NAME_SANITIZED ${PLUGIN_NAME})
            string (APPEND DECLARE_FUNCTIONS "    void RegisterPlugin_${PLUGIN_NAME_SANITIZED}();\n")
            string (APPEND REGISTER_PLUGINS "    RegisterPlugin_${PLUGIN_NAME_SANITIZED}();\n")
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
                    COMPONENT ThirdParty
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
                # Also add to main Urho3D export for backward compatibility
                install (TARGETS ${TARGET}
                    EXPORT Urho3D
                    COMPONENT ThirdParty
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
            endif ()
        endforeach ()
    endif ()
endfunction ()

function (install_tools)
    foreach (TARGET ${ARGV})
        if (TARGET ${TARGET})
            # Only executables are installed to Urho3DTools
            get_target_property(TARGET_TYPE ${TARGET} TYPE)
            if (TARGET_TYPE STREQUAL "EXECUTABLE")
                install (TARGETS ${TARGET}
                    EXPORT Urho3DTools
                    COMPONENT Tools
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    PERMISSIONS ${PERMISSIONS_755})
            endif ()
            # Also add to main Urho3D export for backward compatibility
            install (TARGETS ${TARGET}
                EXPORT Urho3D
                COMPONENT Tools
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                PERMISSIONS ${PERMISSIONS_755})
        endif ()
    endforeach ()
endfunction ()

function (install_third_party_tools)
    install_tools(${ARGV})
    foreach (TARGET ${ARGV})
        if (TARGET ${TARGET})
            install (TARGETS ${TARGET}
                EXPORT Urho3DThirdParty
                COMPONENT ThirdParty
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
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

# Disables hot-reloading of plugins when building specified targets.
# Always build [all] target if this feature is used!
function(disable_reload_when_building)
    if(NOT TARGET LockBinaries)
        set(MARKER_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/.noreload")
        add_custom_target(LockBinaries COMMAND ${CMAKE_COMMAND} -E touch ${MARKER_FILE} COMMENT "Disabling hot-reloading for binaries")
        add_custom_target(UnlockBinaries ALL COMMAND ${CMAKE_COMMAND} -E rm -f ${MARKER_FILE} COMMENT "Re-enabling hot-reloading for binaries")
    endif()

    foreach(target IN LISTS ARGN)
        add_dependencies(${target} LockBinaries)
        add_dependencies(UnlockBinaries ${target})
    endforeach()
endfunction()
