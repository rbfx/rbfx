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


# Pre-generate part of swig interface

set_source_files_properties(Swig/${BIND_TARGET}.i PROPERTIES
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_constants.i
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_refcounted.i
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_enums.i
    )
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_constants.i
    ${CMAKE_CURRENT_BINARY_DIR}/_refcounted.i
    ${CMAKE_CURRENT_BINARY_DIR}/_enums.i
    COMMAND python3 "${Urho3D_SOURCE_DIR}/Source/Tools/AutoSwig/autoswig.py" "${CSHARP_BINDING_GENERATOR_OPTIONS}"
    COMMENT Generating rest of swig interface
)

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
