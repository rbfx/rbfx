#
# Copyright (c) 2017-2022 the rbfx project.
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

# Always export Player without entry point as PlayerLibrary.
add_library (PlayerLibrary STATIC PlayerApplication.cpp PlayerApplication.h)
target_include_directories(PlayerLibrary PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${DEST_INCLUDE_DIR}/Tools>
)
target_link_libraries(PlayerLibrary PUBLIC Urho3D)

if (NOT URHO3D_NO_EDITOR_PLAYER_EXE)
    if (ANDROID OR URHO3D_CSHARP)
        add_library (libPlayer MODULE ${SOURCE_FILES} Player.cpp)
        set_target_properties(libPlayer PROPERTIES PREFIX "")
        set (PLAYER_TARGET libPlayer)
        if (URHO3D_CSHARP)
            add_target_csharp(
                TARGET Player
                EXE
                PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/ManagedHost/Player.csproj
                OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Player
                DEPENDS Urho3DNet)
            if (TARGET Player)
                add_dependencies(Player libPlayer Urho3DNet)
            endif ()
            install(FILES "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Player${CMAKE_EXECUTABLE_SUFFIX}" DESTINATION ${DEST_BIN_DIR_CONFIG})
        endif ()
    else ()
        if (NOT URHO3D_WIN32_CONSOLE)
            set (TARGET_TYPE WIN32)
        endif ()
        add_executable (Player ${TARGET_TYPE} Player.cpp)
        set (PLAYER_TARGET Player)
    endif ()

    target_link_libraries(${PLAYER_TARGET} PUBLIC PlayerLibrary)
    target_link_plugins (${PLAYER_TARGET} "${URHO3D_PLUGIN_LIST}")
endif ()

# SDK installation
install (TARGETS ${PLAYER_TARGET}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
    BUNDLE  DESTINATION ${DEST_BIN_DIR_CONFIG}
)
install (TARGETS PlayerLibrary EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${DEST_INCLUDE_DIR}/Tools
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
