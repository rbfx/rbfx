# Copyright (c) 2017-2022 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT URHO3D_CSHARP OR NOT URHO3D_PLUGINS OR NOT URHO3D_PLAYER)
    return ()
endif ()

add_target_csharp(
    TARGET 104_CSharpPlugin
    PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/104_CSharpPlugin.csproj
    OUTPUT ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/104_CSharpPlugin
    DEPENDS Urho3DNet)
install (FILES ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/104_CSharpPlugin.dll DESTINATION ${CMAKE_INSTALL_BINDIR})
