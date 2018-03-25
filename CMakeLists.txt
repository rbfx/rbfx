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

project(Urho3DNet)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(ThirdParty)
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

macro (csharp_bind_target TARGET SOURCE_DIR)
    set (CSHARP_DIR ${ARGN})

    if (NOT SOURCE_DIR)
        get_target_property(SOURCE_DIR ${TARGET} SOURCE_DIR)
    endif ()

    if (NOT CSHARP_DIR)
        set (CSHARP_DIR ${Urho3DNet_SOURCE_DIR}/CSharp/${TARGET})
        if (NOT IS_DIRECTORY ${CSHARP_DIR})
            unset (CSHARP_DIR)
        endif ()
    endif ()

    if (CSHARP_DIR)
        set (RULES ${CSHARP_DIR}/config-csharp.json)
    endif ()

    if (NOT EXISTS ${RULES})
        set (RULES ${SOURCE_DIR}/config-csharp.json)
    endif ()

    if (NOT EXISTS ${RULES})
        message(FATAL_ERROR "config-csharp.json was not found. It must reside in SOURCE_DIR or CSHARP_DIR.")
    endif ()

    # Parse bindings using same compile definitions as built target
    __TARGET_GET_PROPERTIES_RECURSIVE(INCLUDES ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    __TARGET_GET_PROPERTIES_RECURSIVE(DEFINES  ${TARGET} INTERFACE_COMPILE_DEFINITIONS)
    __TARGET_GET_PROPERTIES_RECURSIVE(OPTIONS  ${TARGET} INTERFACE_COMPILE_OPTIONS)
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
        list(APPEND GENERATOR_OPTIONS -I ${item})
    endforeach()
    foreach(item ${DEFINES})
        list(APPEND GENERATOR_OPTIONS -D ${item})
    endforeach()
    foreach(item ${OPTIONS})
        list(APPEND GENERATOR_OPTIONS -O _${item})
    endforeach()
    set (BINDINGS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Net)
    set (COMMON_DIR ${Urho3DNet_SOURCE_DIR}/CSharp/Common)
    list (APPEND GENERATOR_OPTIONS ${RULES} ${SOURCE_DIR} ${BINDINGS_OUTPUT_DIR})

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/generator_commandline.txt "")
    foreach (opt ${GENERATOR_OPTIONS})
        file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/generator_commandline.txt "${opt}\n")
    endforeach ()
    add_custom_target(CSharpGenerateBindings
        COMMAND $<TARGET_FILE:CSharpGenerator> ${CMAKE_CURRENT_BINARY_DIR}/generator_commandline.txt
        DEPENDS CSharpGenerator
    )

    # CSharp target (native)
    file (GLOB COMMON_CSHARP_SOURCE_FILES_MANUAL
        ${COMMON_DIR}/Native/*.cpp ${CSHARP_DIR}/Native/*.cpp)
    set (COMMON_CSHARP_SOURCE_FILES_GENERATED
        ${BINDINGS_OUTPUT_DIR}/Native/CApi.cpp
        ${BINDINGS_OUTPUT_DIR}/Native/ClassWrappers.hpp
        ${BINDINGS_OUTPUT_DIR}/Native/RegisterFactories.cpp
    )
    set_source_files_properties(${COMMON_CSHARP_SOURCE_FILES_GENERATED} PROPERTIES GENERATED TRUE)
    add_library(${TARGET}CSharp SHARED ${COMMON_CSHARP_SOURCE_FILES_MANUAL} ${COMMON_CSHARP_SOURCE_FILES_GENERATED})
    target_link_libraries(${TARGET}CSharp ${TARGET})
    target_include_directories(${TARGET}CSharp PRIVATE
        ${COMMON_DIR}/Native ${CSHARP_DIR}/Native ${BINDINGS_OUTPUT_DIR}/Native)
    add_dependencies(${TARGET}CSharp CSharpGenerateBindings)

    # Net target (managed)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (NET_PLATFORM /platform:x64)
    elseif (DESKTOP)
        set (NET_PLATFORM /platform:x86)
    endif()

    if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
        # Workaround for some cases where csc has issues when invoked by CMake.
        set (TERM_WORKAROUND env TERM=xterm)
    endif ()

    file (GLOB_RECURSE COMMON_NET_SOURCE_FILES_MANUAL ${COMMON_DIR}/CSharp/*.cs ${CSHARP_DIR}/CSharp/*.cs)
    set (COMMON_NET_SOURCE_FILES_GENERATED
        ${BINDINGS_OUTPUT_DIR}/CSharp/PInvoke.cs
        ${BINDINGS_OUTPUT_DIR}/CSharp/CSharp.cs
    )
    find_program(CSC csc)
    if (NOT CSC)
        message(FATAL_ERROR "C# compiler could not be found.")
    endif ()
    if (MSVC)
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    else ()
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif ()
    add_custom_target(${TARGET}Net
        COMMAND ${TERM_WORKAROUND} ${CSC} /target:library ${NET_PLATFORM} /unsafe /reference:System.dll
                /out:${NET_OUTPUT_DIRECTORY}/${TARGET}Net.dll
                ${COMMON_NET_SOURCE_FILES_MANUAL} ${COMMON_NET_SOURCE_FILES_GENERATED}
    )
    add_dependencies(${TARGET}Net ${TARGET}CSharp)
endmacro ()
