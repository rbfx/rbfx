#
# Copyright (c) 2017-2020 the rbfx project.
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
cmake_policy(PUSH)
cmake_policy(SET CMP0009 NEW)

# CMake is not aware of files C# builds produce. Since those files end up in
# binary directory we inconditionally install any possible C# artifacts.
file (GLOB_RECURSE INSTALL_FILES
    RELATIVE ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/*.dll
    ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/*.exe
    ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/*.exe.config
    ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/*.pdb
)

foreach (file ${INSTALL_FILES})
    set (SOURCE_FILE ${CMAKE_BINARY_DIR}/${file})
    set (DEST_FILE ${CMAKE_INSTALL_PREFIX}/${file})
    if (NOT EXISTS "${DEST_FILE}")
        get_filename_component(DEST_DIR "${DEST_FILE}" DIRECTORY)
        file (MAKE_DIRECTORY "${DEST_DIR}")
        file (COPY ${SOURCE_FILE} DESTINATION "${DEST_DIR}")
    endif ()
endforeach()

cmake_policy(POP)
