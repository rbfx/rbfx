# Copyright (c) 2017-2022 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT URHO3D_CSHARP)
    return ()
endif ()

return_if_not_tool(ScriptPlayer)

add_target_csharp(
    TARGET ScriptPlayer
    EXE
    PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/ScriptPlayer.csproj
    OUTPUT ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/ScriptPlayer
    DEPENDS Urho3DNet)
if (TARGET ScriptPlayer)
    add_dependencies(ScriptPlayer Urho3DNet)
endif ()
install(FILES "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/ScriptPlayer${CMAKE_EXECUTABLE_SUFFIX}" DESTINATION ${CMAKE_INSTALL_BINDIR})
