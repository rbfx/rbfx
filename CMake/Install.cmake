# Copyright (c) 2017-2022 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

cmake_policy(PUSH)
cmake_policy(SET CMP0009 NEW)

# CMake is not aware of files C# builds produce. Since those files end up in
# binary directory we inconditionally install any possible C# artifacts.
file (GLOB_RECURSE INSTALL_FILES
    RELATIVE ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/bin/*.dll
    ${CMAKE_BINARY_DIR}/bin/*.exe
    ${CMAKE_BINARY_DIR}/bin/*.exe.config
    ${CMAKE_BINARY_DIR}/bin/*.deps.json
    ${CMAKE_BINARY_DIR}/bin/*.runtimeconfig.json
    ${CMAKE_BINARY_DIR}/bin/*.pdb
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
