# Copyright (c) 2008-2022 the Urho3D project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT DESKTOP)
    install(FILES PackageTool/PackageTool.py DESTINATION ${CMAKE_INSTALL_BINDIR_BASE})
    return ()
endif ()

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Tools)

add_subdirectory(PackageTool)
add_subdirectory(RampGenerator)
add_subdirectory(SpritePacker)
add_subdirectory(ScriptPlayer)

vs_group_subdirectory_targets(${CMAKE_CURRENT_SOURCE_DIR} Tools)
