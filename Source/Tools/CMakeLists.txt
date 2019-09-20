#
# Copyright (c) 2008-2019 the Urho3D project.
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

if (CMAKE_CROSSCOMPILING AND (WEB OR MOBILE))
    include (ExternalProject)
    if (IOS OR TVOS)
        # When cross-compiling for iOS/tvOS the host environment has been altered by xcodebuild for the said platform, the following fix is required to reset the host environment before spawning another process to configure/generate project file for external project
        # Also workaround a known CMake/Xcode generator bug which prevents it from installing native tool binaries correctly
        set (ALTERNATE_COMMAND /usr/bin/env -i PATH=$ENV{PATH} CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND} BUILD_COMMAND bash -c "sed -i '' 's/\$$\(EFFECTIVE_PLATFORM_NAME\)//g' CMakeScripts/install_postBuildPhase.make*")
    else ()
        set (ALTERNATE_COMMAND ${CMAKE_COMMAND} -E env CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND})
    endif ()
    ExternalProject_Add (Urho3D-Native
        SOURCE_DIR ${rbfx_SOURCE_DIR}
        CMAKE_COMMAND ${ALTERNATE_COMMAND}
        CMAKE_ARGS -DURHO3D_ENABLE_ALL=OFF -DMINI_URHO=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/native PackageTool
    )
    set (PACKAGE_TOOL "${CMAKE_BINARY_DIR}/native/bin/PackageTool" CACHE STRING "" FORCE)
    return ()
endif ()

# Urho3D tools
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${DEST_TOOLS_DIR})

if (URHO3D_TOOLS AND DESKTOP)
    add_subdirectory (PackageTool)
    add_subdirectory (Toolbox)
    add_subdirectory (AssetImporter)
    add_subdirectory (AssetViewer)
    add_subdirectory (OgreImporter)
    add_subdirectory (RampGenerator)
    add_subdirectory (SpritePacker)
    add_subdirectory (Editor)
    add_subdirectory (ScriptPlayer)
    add_subdirectory (SerializationConverter)
elseif (MINI_URHO OR WEB OR MOBILE)
    add_subdirectory (PackageTool)
endif ()

set (PACKAGE_TOOL "$<TARGET_FILE:PackageTool>" CACHE STRING "" FORCE)

vs_group_subdirectory_targets(${CMAKE_CURRENT_SOURCE_DIR} Tools)
