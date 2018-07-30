#
# Copyright (c) 2018 Rokas Kupstys.
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

find_package(SWIG REQUIRED)
include(UrhoSWIG)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

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

macro (add_target_csharp TARGET PROJECT_FILE)
    if (WIN32 AND NOT URHO3D_WITH_MONO)
        include_external_msproject(${TARGET} ${PROJECT_FILE} TYPE FAE04EC0-301F-11D3-BF4B-00C04F79EFBC CSharpBindings)
    else ()
        if (WIN32)
            find_package(Mono REQUIRED)
            find_program(MSBUILD xbuild.bat PATHS ${MONO_PATH}/bin)
        elseif (APPLE)
            find_program(MSBUILD xbuild)
        else ()
            find_program(MSBUILD msbuild)
        endif ()
        if (NOT MSBUILD)
            message(FATAL_ERROR "MSBuild could not be found.")
        endif ()
        if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux")
            # Workaround for some cases where csc has issues when invoked by CMake.
            set (TERM_WORKAROUND env TERM=xterm)
        endif ()
        if (${CMAKE_BUILD_TYPE})
            set (CSHARP_BUILD_TYPE ${CMAKE_BUILD_TYPE})
        else ()
            set (CSHARP_BUILD_TYPE $<CONFIG>)
        endif ()

        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set (CSHARP_PLATFORM x64)
        else ()
            set (CSHARP_PLATFORM x86)
        endif ()

        add_custom_target(${TARGET} COMMAND ${TERM_WORKAROUND} ${MSBUILD} ${PROJECT_FILE} ${ARGN}
            /p:BuildDir="${CMAKE_BINARY_DIR}"
            /p:Configuration=${CSHARP_BUILD_TYPE} /p:Platform=${CSHARP_PLATFORM} /consoleloggerparameters:ErrorsOnly)
        set_target_properties(${TARGET} PROPERTIES EXCLUDE_FROM_ALL OFF)
    endif ()
endmacro ()

macro (csharp_bind_target)
    if (NOT URHO3D_CSHARP2)
        return ()
    endif ()

    cmake_parse_arguments(BIND "" "TARGET;MANAGED_TARGET" "" ${ARGN})

    get_target_property(BIND_SOURCE_DIR ${BIND_TARGET} SOURCE_DIR)

    if (NOT BIND_MANAGED_TARGET)
        set (BIND_MANAGED_TARGET ${BIND_TARGET}Net)
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
        list(APPEND GENERATOR_OPTIONS -O${item})
    endforeach()

#    get_target_property(_TARGET_TYPE ${BIND_TARGET} TYPE)
#    if(_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
#        list (APPEND GENERATOR_OPTIONS --static)
#    endif ()
#    if (SNK_PUB_KEY)
#        list (APPEND GENERATOR_OPTIONS --signed-with=${SNK_PUB_KEY})
#    endif ()

    set (CSHARP_LIBRARY_NAME ${BIND_TARGET}CSharp)
    set (COMMON_DIR ${CSHARP_SOURCE_DIR}/Common)

    # Finalize option list
    list (APPEND GENERATOR_OPTIONS ${BIND_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
    set (CSHARP_BINDING_GENERATOR_OPTIONS "${CMAKE_CURRENT_BINARY_DIR}/generator_options.txt")
    file (WRITE ${CSHARP_BINDING_GENERATOR_OPTIONS} "")
    foreach (opt ${GENERATOR_OPTIONS})
        file(APPEND ${CSHARP_BINDING_GENERATOR_OPTIONS} "${opt}\n")
    endforeach ()

    # Swig
    set(CMAKE_SWIG_FLAGS
        -v
        -namespace ${BIND_MANAGED_TARGET}
#        -debug-tmused
        -debug-tmsearch
        -I${CMAKE_CURRENT_BINARY_DIR}
    )

    set (SWIG_SYSTEM_INCLUDE_DIRS "${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES};${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES};${CMAKE_SYSTEM_INCLUDE_PATH};${CMAKE_EXTRA_GENERATOR_CXX_SYSTEM_INCLUDE_DIRS}")
    string (REPLACE ";" ";-I" SWIG_SYSTEM_INCLUDE_DIRS "${SWIG_SYSTEM_INCLUDE_DIRS}")

    set_source_files_properties(${BIND_TARGET}.i PROPERTIES
        CPLUSPLUS ON
        SWIG_FLAGS "-I${SWIG_SYSTEM_INCLUDE_DIRS}"
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_constants.i
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_refcounted.i
    )

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(../..)

    swig_add_library(${CSHARP_LIBRARY_NAME} TYPE SHARED LANGUAGE csharp SOURCES ${BIND_TARGET}.i)
    swig_link_libraries(${CSHARP_LIBRARY_NAME} ${BIND_TARGET})
    set_target_properties(${CSHARP_LIBRARY_NAME} PROPERTIES PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})

    # Pre-generate part of swig interface
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_constants.i
               ${CMAKE_CURRENT_BINARY_DIR}/_refcounted.i
        COMMAND python3 "${Urho3D_SOURCE_DIR}/Source/Tools/AutoSwig/autoswig.py" "${CSHARP_BINDING_GENERATOR_OPTIONS}"
        COMMENT Generating rest of swig interface
    )
    # Swig gets confused and generates unused classes sometimes. Remove them.
    add_custom_command(TARGET ${CSHARP_LIBRARY_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/SWIGTYPE_p_${BIND_TARGET}__StringHash.cs
    )

    # Etc
    #    if (CLANG)
    #        target_compile_options(${CSHARP_LIBRARY_NAME} PRIVATE -Wno-return-type-c-linkage)
    #    endif ()
    #
    #    if (URHO3D_WITH_MONO)
    #        find_package(Mono REQUIRED)
    #        target_include_directories(${CSHARP_LIBRARY_NAME} PRIVATE ${MONO_INCLUDE_DIRS})
    #        target_link_libraries(${CSHARP_LIBRARY_NAME} ${MONO_LIBRARIES})
    #        target_compile_options(${CSHARP_LIBRARY_NAME} PRIVATE ${MONO_CFLAGS})
    #    endif ()

    if (MSVC)
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
    else ()
        set (NET_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        # Needed for mono on unixes but not on windows.
        set (FACADES Facades/)
    endif ()
    if (EXISTS "${BIND_MANAGED_TARGET}.csproj")
        add_target_csharp(${BIND_MANAGED_TARGET} ${BIND_MANAGED_TARGET}.csproj)
        add_dependencies(${BIND_MANAGED_TARGET} ${CSHARP_LIBRARY_NAME})
        install (FILES ${NET_OUTPUT_DIRECTORY}/${BIND_MANAGED_TARGET}.dll DESTINATION ${DEST_LIBRARY_DIR})
    endif ()

endmacro ()

csharp_bind_target(TARGET Urho3D)

## Target for manually refreshing binding code
#add_custom_target(CSharpBindings
#    COMMAND $<TARGET_FILE:CSharpGenerator> ${CSHARP_BINDING_GENERATOR_OPTIONS}
#    DEPENDS CSharpGenerator
#    COMMENT "Generating CSharp bindings")

## Dependencies for bindings
#file (DOWNLOAD https://www.nuget.org/api/v2/package/System.Buffers/4.4.0 ${CMAKE_BINARY_DIR}/System.Buffers/pkg.zip)
#execute_process(
#    COMMAND ${CMAKE_COMMAND} -E tar "xf" "pkg.zip" --format=zip
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/System.Buffers
#)
#
#if (WIN32)
#    add_custom_command(TARGET CSharpBindings POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/System.Buffers/ref/netstandard1.1/System.Buffers.dll ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/)
#    if (URHO3D_WITH_MONO)
#        add_custom_command(TARGET CSharpBindings POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MONO_PATH}/lib/mono/4.5/Facades/System.Runtime.dll ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/)
#        install (FILES
#            ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/System.Runtime.dll
#            DESTINATION ${DEST_LIBRARY_DIR}
#            )
#    endif ()
#    install (FILES
#        ${CMAKE_BINARY_DIR}/${DEST_LIBRARY_DIR_CONFIG}/System.Buffers.dll
#        DESTINATION ${DEST_LIBRARY_DIR}
#        )
#endif ()
#
#find_program(SN sn)
#if (NOT EXISTS ${CMAKE_BINARY_DIR}/CSharp.snk)
#    execute_process(COMMAND ${SN} -k ${CMAKE_BINARY_DIR}/CSharp.snk)
#endif ()
#if (NOT EXISTS ${CMAKE_BINARY_DIR}/CSharp.snk.pub)
#    execute_process(COMMAND ${SN} -p ${CMAKE_BINARY_DIR}/CSharp.snk ${CMAKE_BINARY_DIR}/CSharp.snk.pub)
#endif ()
#
#execute_process(
#    COMMAND ${SN} -tp ${CMAKE_BINARY_DIR}/CSharp.snk.pub
#    OUTPUT_VARIABLE SNK_PUB_KEY
#)
#string(REGEX MATCH "Public [Kk]ey(.+)?:[0-9a-f\r\n]+\r?\n\r?\n" SNK_PUB_KEY "${SNK_PUB_KEY}")
#string(REGEX REPLACE "Public [Kk]ey(.+)?:" "" SNK_PUB_KEY "${SNK_PUB_KEY}")
#string(REGEX REPLACE "[ \r\n]+" "" SNK_PUB_KEY "${SNK_PUB_KEY}")
#set(SNK_PUB_KEY "${SNK_PUB_KEY}" CACHE STRING "Public key for .NET assemblies" FORCE)
