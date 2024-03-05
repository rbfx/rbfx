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

if (NOT URHO3D_SYSTEMUI OR NOT DESKTOP)
    return ()
endif ()

file (GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.h *.hpp)
list (REMOVE_ITEM SOURCE_FILES Editor.cpp EditorWrapper.cpp)

# Always export Editor without entry point as EditorLibrary.
# TODO: Support SHARED EditorLibrary for external Editor plugins.
add_library (EditorLibrary STATIC ${SOURCE_FILES})

target_include_directories(EditorLibrary PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${DEST_INCLUDE_DIR}/Tools>
)
target_link_libraries (EditorLibrary PUBLIC Urho3D)
if (TARGET TracyEmbedded)
    target_link_libraries (EditorLibrary PUBLIC TracyEmbedded)
endif ()

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
            OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/EditorManaged
            DEPENDS Urho3DNet)

        if (TARGET EditorManaged)
            add_dependencies(EditorManaged libEditorWrapper Urho3DNet)
            install(FILES "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/EditorManaged${CMAKE_EXECUTABLE_SUFFIX}"
                DESTINATION ${DEST_BIN_DIR_CONFIG}
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
install (TARGETS ${EDITOR_TARGETS}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)
install (TARGETS EditorLibrary EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${DEST_INCLUDE_DIR}/Tools
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
