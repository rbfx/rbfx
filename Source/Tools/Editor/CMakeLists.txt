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

if (NOT URHO3D_SYSTEMUI)
    return ()
endif ()

file (GLOB_RECURSE SOURCE_FILES *.cpp *.h *.hpp)

if (URHO3D_NO_EDITOR_PLAYER_EXE)
    # User is going to build their own editor executable
    add_library (libEditor STATIC ${SOURCE_FILES})
    set (EDITOR_TARGET libEditor)
else ()
    if (URHO3D_CSHARP)
        # Use C# executable to host .net runtime and load editor. Editor is built as a dynamic library.
        add_target_csharp(
            TARGET Editor
            EXE
            PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/ManagedHost/Editor.csproj
            OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Editor${CMAKE_EXECUTABLE_SUFFIX}
            DEPENDS Urho3DNet)
        add_library (libEditor SHARED ${SOURCE_FILES} EditorHost.cpp)
        set (EDITOR_TARGET libEditor)
        if (TARGET Editor)
            add_dependencies(Editor libEditor Urho3DNet)
        endif ()
        install(FILES "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Editor${CMAKE_EXECUTABLE_SUFFIX}" DESTINATION ${DEST_BIN_DIR_CONFIG})
    else ()
        # No C# or pugins support, editor can be a true native executable.
        if (NOT URHO3D_WIN32_CONSOLE)
            set (TARGET_TYPE WIN32)
        endif ()
        add_executable (Editor ${TARGET_TYPE} ${SOURCE_FILES} EditorHost.cpp)
        set (EDITOR_TARGET Editor)
    endif ()
endif ()

if (TARGET libEditor)
    set_target_properties(libEditor PROPERTIES PREFIX "")
endif ()

target_include_directories(${EDITOR_TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries (${EDITOR_TARGET} Toolbox Urho3D)
if (TARGET TracyEmbedded)
    target_link_libraries (${EDITOR_TARGET} TracyEmbedded)
endif ()

if (NOT BUILD_SHARED_LIBS AND URHO3D_SAMPLES)
    # Static plugins have to be linked into target application.
    target_link_libraries(${EDITOR_TARGET} 103_GamePlugin)
endif ()

install(TARGETS ${EDITOR_TARGET}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)
