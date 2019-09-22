#
# Copyright (c) 2017-2019 the rbfx project.
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

if (MINI_URHO OR WEB OR MOBILE)
    return ()
endif ()

file (GLOB SOURCE_FILES *.cpp *.h)

if (URHO3D_NO_EDITOR_PLAYER_EXE)
    add_library (libPlayer STATIC ${SOURCE_FILES})
    set (PLAYER_TARGET libPlayer)
elseif (ANDROID OR URHO3D_CSHARP)
    add_library (libPlayer MODULE ${SOURCE_FILES} PlayerHost.cpp)
    set (PLAYER_TARGET libPlayer)
    if (URHO3D_CSHARP)
        add_target_csharp(
            TARGET Player
            PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/ManagedHost/Player.csproj
            OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Player.exe
            DEPENDS Urho3DNet)
    endif ()
else ()
    if (NOT URHO3D_WIN32_CONSOLE)
        set (TARGET_TYPE WIN32)
    endif ()
    add_executable (Player ${TARGET_TYPE} ${SOURCE_FILES} PlayerHost.cpp)
    set (PLAYER_TARGET Player)
endif ()

if (TARGET libPlayer)
    set_target_properties(libPlayer PROPERTIES PREFIX "")
    target_include_directories(libPlayer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

target_link_libraries(${PLAYER_TARGET} Urho3D)

if (URHO3D_SAMPLES AND NOT BUILD_SHARED_LIBS)
    target_link_libraries(${PLAYER_TARGET} 103_GamePlugin)
endif ()

install(TARGETS ${PLAYER_TARGET}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)
