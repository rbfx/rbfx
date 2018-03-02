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

set (CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(ThirdParty)
add_subdirectory(Generator)

macro (CSHARP_BIND_TARGET TARGET RULES)
    get_target_property(INCLUDES ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(DEFINES  ${TARGET} INTERFACE_COMPILE_DEFINITIONS)
    get_target_property(OPTIONS  ${TARGET} INTERFACE_COMPILE_OPTIONS)
    get_target_property(SOURCE_DIR ${TARGET} SOURCE_DIR)
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
    list (APPEND GENERATOR_OPTIONS
        --out-cpp ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}CSharp
        --out-cs ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Net
    )
    list (APPEND GENERATOR_OPTIONS ${RULES})
    list (APPEND GENERATOR_OPTIONS ${SOURCE_DIR})
    list (REMOVE_DUPLICATES GENERATOR_OPTIONS)

    add_custom_target(CSharpGenerateBindings
        COMMAND $<TARGET_FILE:CSharpGenerator> ${GENERATOR_OPTIONS}
        DEPENDS CSharpGenerator
    )

    # CSharp target (native)
    file (GLOB COMMON_CSHARP_SOURCE_FILES_MANUAL CommonCSharp/*.cpp CommonCSharp/*.h)
    set (COMMON_CSHARP_SOURCE_FILES_GENERATED
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}CSharp/CApi.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}CSharp/ClassWrappers.hpp
    )
    set_source_files_properties(${COMMON_CSHARP_SOURCE_FILES_GENERATED} PROPERTIES GENERATED TRUE)
    add_library(${TARGET}CSharp SHARED ${COMMON_CSHARP_SOURCE_FILES_MANUAL} ${COMMON_CSHARP_SOURCE_FILES_GENERATED})
    target_link_libraries(${TARGET}CSharp Urho3D)
    target_include_directories(${TARGET}CSharp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CommonCSharp)
    add_dependencies(${TARGET}CSharp CSharpGenerateBindings)

    # Net target (managed)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (NET_PLATFORM /platform:x64)
    elseif (DESKTOP)
        set (NET_PLATFORM /platform:x86)
    endif()

    file (GLOB_RECURSE COMMON_NET_SOURCE_FILES_MANUAL CommonNet/*.cs ${TARGET}Net/*.cs)
    set (COMMON_NET_SOURCE_FILES_GENERATED
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Net/PInvoke.cs
        ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}Net/Urho3D.cs
    )
    add_custom_target(${TARGET}Net
        COMMAND csc /target:library ${NET_PLATFORM} /unsafe /reference:System.dll
                /out:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}Net.dll
                ${COMMON_NET_SOURCE_FILES_MANUAL} ${COMMON_NET_SOURCE_FILES_GENERATED}
    )
    add_dependencies(${TARGET}Net ${TARGET}CSharp)
endmacro ()

CSHARP_BIND_TARGET(Urho3D ${CMAKE_CURRENT_SOURCE_DIR}/Rules.json)
