# CPM.Cached.cmake
# ================
# This script is a proof of concept for caching CPM packages.
# It is not intended for production use.
#
# This script is a drop-in replacement for CPM.cmake.
# It adds a new function `CPMAddPackageCached` that caches the built package.
# The cache is used when the package is requested again.
#
# The script can be configured using following environment variables:
# - `CPM_BINARY_CACHE_DIR`: The directory where the cached packages are stored. Defaults to `~/.cpm_cache`.
#     Users are advised to use unique cache directories for different projects.
# - `CPM_BINARY_CACHE_ALWAYS_INSTALL`: If set, the package is always built and installed. For debugging only.
# - `CPM_BINARY_CACHE_DISABLED`: If set, package caching is skipped and CPMAddPackageCached forwards to
#     CPMAddPackage.
# - `CPM_BINARY_CACHE_USE_VARS`: If set, the script forwards all specified variables to the dependency builds.
#
# The script also can be configured using the following CMake variables:
# - `CMAKE_CONFIGURATION_TYPES`: This option can be set to reduce a number of different build configurations used.
# - `CMAKE_BUILD_TYPE`: The build type should be set for single-config generators. If not set, defaults to first
#     available build type in CMAKE_CONFIGURATION_TYPES.
#
# The script forwards following environment variables to dependency builds:
# - `CMAKE_TOOLCHAIN_FILE`: The toolchain file.
# - `CMAKE_GENERATOR`: The generator.
# - `CMAKE_GENERATOR_PLATFORM`: The generator platform.
#

option(CPM_BINARY_CACHE_DIR "The directory where the cached packages are stored" "$ENV{CPM_BINARY_CACHE_DIR}")
option(CPM_BINARY_CACHE_DISABLED "Disable CPM binary cache" "$ENV{CPM_BINARY_CACHE_DISABLED}")
set(CPM_BINARY_CACHE_USE_VARS "$ENV{CPM_BINARY_CACHE_USE_VARS}" CACHE STRING "Forward these variables to the dependency build generation")

# We handle this ourselves at the end of CPMAddPackageCached.
set(CPM_DONT_UPDATE_MODULE_PATH ON)
include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)

if (NOT CPM_PACKAGES)
    if (CPM_BINARY_CACHE_DISABLED)
        message(STATUS "CPM binary cache is disabled.")
    else ()
        message(STATUS "CPM binary cache is enabled.")
        message(STATUS "CPM binary cache directory: ${CPM_BINARY_CACHE_DIR}")
    endif ()
    message(STATUS "CPM source cache directory: ${CPM_SOURCE_CACHE}")
endif ()

# Expose CPM-added packages to find_package(). This may not be entirely correct, as
# CMAKE_MODULE_PATH will provide imported targets instead of library information
# as variables. Some dependencies may not expect this and we may have to patch them.
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/pkg_prefix)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/pkg_prefix)

# Create a unique signature of a package build. Changes to this function invalidate
# existing caches!
function(cmp_cache_package_hash out)
    # Hash all arguments
    set(hashable_args "${ARGN}")
    cmake_parse_arguments(CPM_ARGS "" "" "PATCHES" "${ARGN}")

    # Hash patch content
    if(DEFINED CPM_ARGS_PATCHES)
        foreach(patch ${CPM_ARGS_PATCHES})
            file(READ "${patch}" patch_content)
            string(SHA256 patch_hash "${patch_content}")
            list(APPEND hashable_args ${patch}=${patch_hash})
        endforeach()
    endif()

    # Hash system name
    list(APPEND hashable_args CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})
    # Hash toolchain file
    if (EXISTS ${CMAKE_TOOLCHAIN_FILE})
        file(READ ${CMAKE_TOOLCHAIN_FILE} toolchain_content)
        string(SHA256 toolchain_hash "${toolchain_content}")
        list(APPEND hashable_args ${CMAKE_TOOLCHAIN_FILE}=${toolchain_hash})
    endif()

    # Multi and single config builds can differ
    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    list(APPEND hashable_args is_multi_config=${is_multi_config})

    #message(STATUS "Hashing package with: ${hashable_args}")

    # Produce package hash
    string(MD5 package_hash "${hashable_args}")
    set(package_hash ${package_hash} PARENT_SCOPE)
endfunction()

# Convenience function that executes a process, prints output and returns error code
# in a LAST_ERROR variable.
function(cpm_cache_exec_process msg)
    message(STATUS "${msg}")
    string(REPLACE ";" " " command_str "${ARGN}")
    message(STATUS "-> ${command_str}")
    execute_process(
        COMMAND ${ARGN}
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
        RESULTS_VARIABLE LAST_ERROR
    )
    set(LAST_ERROR ${LAST_ERROR} PARENT_SCOPE)
endfunction()

# Sets `out` to the cache directory. If not set, the function sets it to a NOTFOUND value.
function(cpm_cache_path out)
    if (NOT CPM_BINARY_CACHE_DIR)
        set(${out} NOTFOUND PARENT_SCOPE)
        return()
    endif ()
    get_filename_component(CPM_CACHE_DIR "${CPM_BINARY_CACHE_DIR}" ABSOLUTE)
    if (NOT CPM_CACHE_DIR)
        if(CMAKE_HOST_WIN32)
            set(CPM_CACHE_DIR $ENV{USERPROFILE}/.cpm_cache)
        else()
            set(CPM_CACHE_DIR $ENV{HOME}/.cpm_cache)
        endif()
    endif()
    set(${out} ${CPM_CACHE_DIR} PARENT_SCOPE)
endfunction()

# A drop-in replacement for CPMAddPackage that caches the built package.
function(CPMAddPackageCached)
    cmake_parse_arguments(CPM_ARGS "" "NAME;DOWNLOAD_ONLY" "PATCHES;OPTIONS" "${ARGN}")

    if (CPM_ARGS_DOWNLOAD_ONLY)
        message(FATAL_ERROR "CPMAddPackageCached does not support DOWNLOAD_ONLY option. Use CPMAddPackage() instead.")
    endif ()

    cmp_cache_package_hash(package_hash ${ARGN})
    set(CPM_PACKAGE_${CPM_ARGS_NAME}_HASH ${package_hash} CACHE INTERNAL "")

    # Check if package is already cached
    if (CPM_BINARY_CACHE_DISABLED)
        # Forward args to CPM, no caching
        CPMAddPackage(${ARGN})
    else ()
        # Derive cache dir.
        cpm_cache_path(CPM_CACHE_DIR)
        set(package_cached_path ${CPM_CACHE_DIR}/${CPM_ARGS_NAME}/${package_hash})

        if (NOT CPM_ARGS_NAME IN_LIST CPM_PACKAGES)
            set(first_time TRUE)
        endif ()

        # Add package, but download-only
        CPMAddPackage(${ARGN} DOWNLOAD_ONLY YES)
        if (first_time AND NOT CPM_BINARY_CACHE_SILENT)
            message(STATUS "-> Binary cache: ${package_cached_path}")
        endif ()

        # Build and install one or more configs.
        # User may set CMAKE_CONFIGURATION_TYPES to reduce amount of configurations used.
        # User may use a single-config generator as well.
        if (NOT EXISTS "${package_cached_path}" OR "$ENV{CPM_BINARY_CACHE_ALWAYS_INSTALL}")
            get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
            if (is_multi_config)
                if (NOT DEFINED CPM_ARGS_BUILD_TYPES)
                    set(CPM_ARGS_BUILD_TYPES ${CMAKE_CONFIGURATION_TYPES})
                endif()
            elseif(CMAKE_BUILD_TYPE)
                set(CPM_ARGS_BUILD_TYPES ${CMAKE_BUILD_TYPE})
            else()
                list(GET CMAKE_CONFIGURATION_TYPES 0 CPM_ARGS_BUILD_TYPES)
                if (NOT CPM_ARGS_BUILD_TYPES)
                    set(CPM_ARGS_BUILD_TYPES "Release")
                endif()
            endif()

            # Build type is not relevant with multiconfig generators.
            if (NOT is_multi_config)
                set(extra_configure_args -DCMAKE_BUILD_TYPE=${CPM_ARGS_BUILD_TYPES})
            endif ()

            # Forward arguments that define build environment to the dependency generator.
            foreach(arg BUILD_SHARED_LIBS CMAKE_TOOLCHAIN_FILE CMAKE_GENERATOR CMAKE_GENERATOR_PLATFORM ${CPM_BINARY_CACHE_USE_VARS})
                if (DEFINED ${arg})
                    list(APPEND extra_configure_args -D${arg}=${${arg}})
                endif()
            endforeach()

            # Generate build dir.
            string(REGEX REPLACE "([a-zA-Z0-9_]+) +([^;]+)" "-D\\1=\\2" CPM_ARGS_OPTIONS "${CPM_ARGS_OPTIONS}")
            cpm_cache_exec_process(
                "Generating ${CPM_ARGS_NAME} build directory:"
                ${CMAKE_COMMAND} -E env CPM_BINARY_CACHE_DIR=${CPM_BINARY_CACHE_DIR} CPM_SOURCE_CACHE=${CPM_SOURCE_CACHE} --
                ${CMAKE_COMMAND} -S ${${CPM_ARGS_NAME}_SOURCE_DIR} -B ${${CPM_ARGS_NAME}_BINARY_DIR}
                                -DCMAKE_MODULE_PATH=${CMAKE_BINARY_DIR}/pkg_prefix -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}/pkg_prefix
                                ${extra_configure_args} ${CPM_ARGS_OPTIONS}
            )
            if (LAST_ERROR)
                message(FATAL_ERROR "Failed to generate build directory for ${CPM_ARGS_NAME}. (${LAST_ERROR})")
            endif()

            # Build and install each configuration.
            foreach(config ${CPM_ARGS_BUILD_TYPES})
                if (is_multi_config)
                    set(extra_build_args --config ${config})
                endif ()

                cpm_cache_exec_process(
                    "Building ${CPM_ARGS_NAME} ${config} configuration:"
                    ${CMAKE_COMMAND} --build ${${CPM_ARGS_NAME}_BINARY_DIR} ${extra_build_args}
                )
                if (LAST_ERROR)
                    message(FATAL_ERROR "Failed to build ${CPM_ARGS_NAME}.")
                endif()

                cpm_cache_exec_process(
                    "Installing ${CPM_ARGS_NAME} ${config} configuration:"
                    ${CMAKE_COMMAND} --install ${${CPM_ARGS_NAME}_BINARY_DIR} --prefix ${package_cached_path} ${extra_build_args}
                )
                if (LAST_ERROR)
                    message(FATAL_ERROR "Failed to install ${CPM_ARGS_NAME}.")
                endif()
            endforeach()
        endif ()
    endif ()

    # Replace CPMAddPackage with CPMAddPackageCached in the the module file used by find_package().
    string(TOLOWER ${CPM_ARGS_NAME} CPM_ARGS_NAME_LOWER)

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/pkg_prefix)
    set(filename "${CMAKE_BINARY_DIR}/pkg_prefix/Find${CPM_ARGS_NAME}.cmake")
    file(WRITE "${filename}"
        "include(${CMAKE_CURRENT_FUNCTION_LIST_FILE})\n"
        "set(CPM_BINARY_CACHE_DIR \"${CPM_BINARY_CACHE_DIR}\")\n"
        "set(CPM_BINARY_CACHE_DISABLED \"${CPM_BINARY_CACHE_DISABLED}\")\n"
        "set(CPM_BINARY_CACHE_USE_VARS \"${CPM_BINARY_CACHE_USE_VARS}\")\n"
        "set(CPM_SOURCE_CACHE \"${CPM_SOURCE_CACHE}\")\n"
        "set(CPM_BINARY_CACHE_SILENT ON)\n"
        "CPMAddPackageCached(\"${ARGN}\")\n"
        "set(${CPM_ARGS_NAME}_FOUND TRUE)\n"
    )
    file(COPY_FILE ${filename} ${CMAKE_BINARY_DIR}/pkg_prefix/${CPM_ARGS_NAME_LOWER}-config.cmake)

    # Include built package.
    # TODO: Name list may not be exhaustive, user may use unconventional names.
    foreach(filename_variant
        "${CPM_ARGS_NAME}Config.cmake"
        "${CPM_ARGS_NAME_LOWER}-config.cmake")
        # This list is definitely not exhaustive. See https://cmake.org/cmake/help/latest/command/find_package.html#id10
        foreach(dir_variant
            ""
            "/cmake"
            "/cmake/${CPM_ARGS_NAME_LOWER}"
            "/lib/cmake"
            "/lib/cmake/${CPM_ARGS_NAME_LOWER}")
            set(config_file_path ${package_cached_path}${dir_variant}/${filename_variant})
            if (EXISTS ${config_file_path})
                include(${config_file_path})
                break()
            endif ()
        endforeach()
    endforeach()
endfunction()
