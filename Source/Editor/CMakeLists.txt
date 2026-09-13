# Copyright (c) 2017-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

if (NOT URHO3D_SYSTEMUI OR NOT DESKTOP)
    return ()
endif ()

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME Tools)

file (GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h *.hpp)
list (REMOVE_ITEM SOURCE_FILES Editor.cpp EditorWrapper.cpp)

#
# Groups sources so that they're displayed in IDE matching their layout in the filesystem
#
group_sources()

# Always export Editor without entry point as EditorLibrary.
# TODO: Support SHARED EditorLibrary for external Editor plugins.
add_library (EditorLibrary STATIC ${SOURCE_FILES})

target_include_directories(EditorLibrary PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Tools>
)
target_link_libraries (EditorLibrary PUBLIC Urho3D)

# Unless forbidden by the user, add Editor executable(s).
set (EDITOR_TARGETS "")
if (NOT URHO3D_NO_EDITOR_PLAYER_EXE)
    # Always add native application
    if (NOT URHO3D_WIN32_CONSOLE)
        set (TARGET_TYPE WIN32)
    endif ()

    add_executable (Editor ${TARGET_TYPE} Editor.cpp)
    target_link_libraries (Editor PUBLIC EditorLibrary)
    list (APPEND EDITOR_TARGETS Editor)

    if (URHO3D_CSHARP)
        # Add native application as shared library to be loaded by C# executable.
        add_library (libEditorWrapper SHARED EditorWrapper.cpp)
        set_target_properties(libEditorWrapper PROPERTIES PREFIX "")
        target_link_libraries (libEditorWrapper PUBLIC EditorLibrary)
        list (APPEND EDITOR_TARGETS libEditorWrapper)

        # Add C# executable to host .net runtime and load editor.
        add_target_csharp(
            TARGET EditorManaged
            EXE
            PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/EditorManaged/EditorManaged.csproj
            OUTPUT ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/EditorManaged
            DEPENDS Urho3DNet)

        if (TARGET EditorManaged)
            add_dependencies(EditorManaged libEditorWrapper Urho3DNet)
            install(FILES "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/EditorManaged${CMAKE_EXECUTABLE_SUFFIX}"
                DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif ()
    else ()
        # No C# or pugins support, editor can be a true native executable.
    endif ()
endif ()

foreach (EDITOR_TARGET ${EDITOR_TARGETS})
    target_link_plugins (${EDITOR_TARGET} "${URHO3D_PLUGIN_LIST}")
endforeach ()

# SDK installation
install_tools(${EDITOR_TARGETS})
foreach (EDITOR_TARGET ${EDITOR_TARGETS})
    install_target_runtime_deps(TARGET ${EDITOR_TARGET} COMPONENT Tools)
endforeach ()
install (TARGETS EditorLibrary EXPORT Urho3D ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Tools
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
