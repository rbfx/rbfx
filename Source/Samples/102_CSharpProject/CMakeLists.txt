# Copyright (c) 2017-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT URHO3D_CSHARP)
    return ()
endif ()

# Workaround for UWP CI builds getting stuck.
if ((NOT "$ENV{CI}" STREQUAL "") AND UWP)
    message(WARNING "102_CSharpProject is excluded from build")
    return ()
endif ()

if (UWP)
    set(NAME_SUFFIX .UWP)
endif ()

add_target_csharp(
    TARGET 102_CSharpProject
    EXE
    PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/102_CSharpProject${NAME_SUFFIX}.csproj
    OUTPUT ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/102_CSharpProject
    DEPENDS Urho3DNet)
install (FILES "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/102_CSharpProject${CMAKE_EXECUTABLE_SUFFIX}" DESTINATION ${CMAKE_INSTALL_BINDIR})
