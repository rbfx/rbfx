#
# Copyright (c) 2008-2018 the Urho3D project.
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

project(CSHARP)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Generator)

macro (__TARGET_GET_PROPERTIES_RECURSIVE OUTPUT TARGET PROPERTY)
    get_target_property(values ${TARGET} ${PROPERTY})
    if (values)
        list (APPEND ${OUTPUT} ${values})
    endif ()
    get_target_property(values ${TARGET} INTERFACE_LINK_LIBRARIES)
    if (values)
        foreach(lib ${values})
            if (TARGET ${lib})
                __TARGET_GET_PROPERTIES_RECURSIVE(${OUTPUT} ${lib} ${PROPERTY})
            endif ()
        endforeach()
    endif()
endmacro()

macro (add_target_csharp TARGET)
    find_program(CSC csc)
    if (NOT CSC)
        find_program(CSC mcs)
        if (NOT CSC)
            message(FATAL_ERROR "C# compiler could not be found.")
        endif ()
    endif ()
    if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
        # Workaround for some cases where csc has issues when invoked by CMake.
        set (TERM_WORKAROUND env TERM=xterm)
    endif ()
    # Net target (managed)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (NET_PLATFORM /platform:x64)
    elseif (DESKTOP)
        set (NET_PLATFORM /platform:x86)
    else ()
        message(FATAL_ERROR "Unsupported platform")
    endif()
    add_custom_target(${TARGET} COMMAND ${TERM_WORKAROUND} ${CSC} ${ARGN} ${NET_PLATFORM})
    set_target_properties(${TARGET} PROPERTIES EXCLUDE_FROM_ALL OFF)
endmacro ()

macro (csharp_bind_target)
    cmake_parse_arguments(BIND "INTERNAL;EXTERNAL" "TARGET;SOURCE_DIR;CSHARP_DIR" "" ${ARGN})

    if (NOT BIND_SOURCE_DIR)
        get_target_property(BIND_SOURCE_DIR ${BIND_TARGET} BIND_SOURCE_DIR)
    endif ()

    if (NOT BIND_CSHARP_DIR)
        set (BIND_CSHARP_DIR ${CSHARP_SOURCE_DIR}/CSharp/${BIND_TARGET})
        if (NOT IS_DIRECTORY ${BIND_CSHARP_DIR})
            unset (BIND_CSHARP_DIR)
        endif ()
    endif ()

    if (BIND_CSHARP_DIR)
        set (RULES ${BIND_CSHARP_DIR}/config-csharp.json)
    endif ()

    if (NOT EXISTS ${RULES})
        set (RULES ${BIND_SOURCE_DIR}/config-csharp.json)
    endif ()

    if (NOT EXISTS ${RULES})
        message(FATAL_ERROR "config-csharp.json was not found. It must reside in BIND_SOURCE_DIR or BIND_CSHARP_DIR.")
    endif ()

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
        list(APPEND GENERATOR_OPTIONS -D${item})
    endforeach()
    foreach(item ${OPTIONS})
        list(APPEND GENERATOR_OPTIONS -O_${item})
    endforeach()

    get_target_property(_TARGET_TYPE ${BIND_TARGET} TYPE)
    if(_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
        list (APPEND GENERATOR_OPTIONS --static)
    endif ()

    set (BINDINGS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BIND_TARGET}Net)
    set (COMMON_DIR ${CSHARP_SOURCE_DIR}/CSharp/Common)
    list (APPEND GENERATOR_OPTIONS ${RULES} ${BIND_SOURCE_DIR} ${BINDINGS_OUTPUT_DIR})

    set (CSHARP_BINDING_GENERATOR_OPTIONS ${CMAKE_CURRENT_BINARY_DIR}/${BIND_TARGET}CSharp_generator_commandline.txt)
    file(WRITE ${CSHARP_BINDING_GENERATOR_OPTIONS} "")
    foreach (opt ${GENERATOR_OPTIONS})
        file(APPEND ${CSHARP_BINDING_GENERATOR_OPTIONS} "${opt}\n")
    endforeach ()

    # Generated source files
    set (CSHARP_SOURCE_FILES_GENERATED
        ${BINDINGS_OUTPUT_DIR}/Native/CApi.cpp
        ${BINDINGS_OUTPUT_DIR}/Native/ClassWrappers.hpp
        ${BINDINGS_OUTPUT_DIR}/Native/RegisterFactories.cpp
    )
    set (COMMON_NET_SOURCE_FILES_GENERATED
        ${BINDINGS_OUTPUT_DIR}/CSharp/PInvoke.cs
        ${BINDINGS_OUTPUT_DIR}/CSharp/CSharp.cs
    )
    set_source_files_properties(${CSHARP_SOURCE_FILES_GENERATED} ${COMMON_NET_SOURCE_FILES_GENERATED} PROPERTIES GENERATED TRUE)

    # CSharp target (native)
    file (GLOB COMMON_CSHARP_SOURCE_FILES_MANUAL
        ${COMMON_DIR}/Native/*.cpp ${BIND_CSHARP_DIR}/Native/*.cpp)


    if (BIND_INTERNAL)
        set (CSHARP_LIBRARY_NAME ${BIND_TARGET})
        target_sources(${BIND_TARGET} PRIVATE ${COMMON_CSHARP_SOURCE_FILES_MANUAL} ${CSHARP_SOURCE_FILES_GENERATED})
    elseif (BIND_EXTERNAL)
        set (CSHARP_LIBRARY_NAME ${BIND_TARGET}CSharp)
        add_library(${CSHARP_LIBRARY_NAME} SHARED ${COMMON_CSHARP_SOURCE_FILES_MANUAL} ${CSHARP_SOURCE_FILES_GENERATED})
        target_link_libraries(${CSHARP_LIBRARY_NAME} ${BIND_TARGET})
        install(TARGETS ${CSHARP_LIBRARY_NAME}
            LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
            RUNTIME DESTINATION ${DEST_BIN_DIR}
            ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
        )
    else ()
        message(FATAL_ERROR "Please pass INTERNAL or EXTERNAL to csharp_bind_target()")
    endif ()
    if (CLANG)
        target_compile_options(${CSHARP_LIBRARY_NAME} PRIVATE -Wno-return-type-c-linkage)
    endif ()
    target_include_directories(${CSHARP_LIBRARY_NAME} PRIVATE
        ${COMMON_DIR}/Native ${BIND_CSHARP_DIR}/Native ${BINDINGS_OUTPUT_DIR}/Native)

    find_package(Mono REQUIRED)
    target_include_directories(${CSHARP_LIBRARY_NAME} PRIVATE ${MONO_INCLUDE_DIRS})
    target_link_libraries(${CSHARP_LIBRARY_NAME} ${MONO_LIBRARIES})
    target_compile_options(${CSHARP_LIBRARY_NAME} PRIVATE ${MONO_CFLAGS})

    # Target that generates bindings source
    add_custom_command(OUTPUT ${CSHARP_SOURCE_FILES_GENERATED} ${COMMON_NET_SOURCE_FILES_GENERATED}
        COMMAND $<TARGET_FILE:CSharpGenerator> ${CSHARP_BINDING_GENERATOR_OPTIONS}
        DEPENDS CSharpGenerator)
    add_custom_target(${CSHARP_LIBRARY_NAME}Regenerate
        COMMAND $<TARGET_FILE:CSharpGenerator> ${CSHARP_BINDING_GENERATOR_OPTIONS}
        DEPENDS CSharpGenerator)
    set_target_properties(${CSHARP_LIBRARY_NAME}Regenerate PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)

    file (WRITE ${BINDINGS_OUTPUT_DIR}/CSharp/Config.cs
"namespace Urho3D.CSharp
{
    internal static partial class Config
    {
        public const string NativeLibraryName = \"${CSHARP_LIBRARY_NAME}\";
    }
}\n")
    file (GLOB_RECURSE COMMON_NET_SOURCE_FILES_MANUAL ${COMMON_DIR}/CSharp/*.cs ${BIND_CSHARP_DIR}/CSharp/*.cs)
    if (MSVC)
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    else ()
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif ()
    add_target_csharp(${BIND_TARGET}Net /target:library /unsafe /reference:System.dll
        /out:${NET_OUTPUT_DIRECTORY}/${BIND_TARGET}Net.dll ${COMMON_NET_SOURCE_FILES_MANUAL}
        ${COMMON_NET_SOURCE_FILES_GENERATED} ${BINDINGS_OUTPUT_DIR}/CSharp/Config.cs
    )
    add_dependencies(${BIND_TARGET}Net ${CSHARP_LIBRARY_NAME})
    install (FILES ${NET_OUTPUT_DIRECTORY}/${BIND_TARGET}Net.dll DESTINATION ${DEST_LIBRARY_DIR})
endmacro ()
