#
# Copyright (c) 2008-2017 the Urho3D project.
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

# Resource packaging
file (GLOB RESOURCE_DIRS ${Urho3D_SOURCE_DIR}/bin/*Data ${CMAKE_SOURCE_DIR}/bin/*Data)
file (GLOB AUTOLOAD_DIRS ${Urho3D_SOURCE_DIR}/bin/Autoload/* ${CMAKE_SOURCE_DIR}/bin/Autoload/*)

# Trim packaged paks from resource dirs
foreach (ITEM ${RESOURCE_DIRS} ${AUTOLOAD_DIRS})
    if ("${ITEM}" MATCHES ".*\\.pak")
        list (REMOVE_ITEM RESOURCE_DIRS "${ITEM}")
        list (REMOVE_ITEM AUTOLOAD_DIRS "${ITEM}")
    endif ()
endforeach ()

if (URHO3D_PACKAGING)
    if (CMAKE_CROSSCOMPILING)
        include (ExternalProject)
        if (IOS OR TVOS)
            # When cross-compiling for iOS/tvOS the host environment has been altered by xcodebuild for the said platform, the following fix is required to reset the host environment before spawning another process to configure/generate project file for external project
            # Also workaround a known CMake/Xcode generator bug which prevents it from installing native tool binaries correctly
            set (ALTERNATE_COMMAND /usr/bin/env -i PATH=$ENV{PATH} CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND} BUILD_COMMAND bash -c "sed -i '' 's/\$$\(EFFECTIVE_PLATFORM_NAME\)//g' CMakeScripts/install_postBuildPhase.make*")
        else ()
            set (ALTERNATE_COMMAND ${CMAKE_COMMAND} -E env CC=${SAVED_CC} CXX=${SAVED_CXX} CI=$ENV{CI} ${CMAKE_COMMAND})
        endif ()
        ExternalProject_Add (Urho3D-Native
            SOURCE_DIR ${Urho3D_SOURCE_DIR}
            CMAKE_COMMAND ${ALTERNATE_COMMAND}
            CMAKE_ARGS -DURHO3D_PACKAGING_TOOL=ON -DMINI_URHO=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/native PackageTool
        )
        set (PACKAGE_TOOL ${CMAKE_BINARY_DIR}/native/bin/PackageTool)
    elseif (TARGET PackageTool)
        set (PACKAGE_TOOL $<TARGET_FILE:PackageTool>)
    else ()
        message(FATAL_ERROR "CMake misconfiguration")
    endif ()

    if (URHO3D_SAMPLES)
        # Package resources for samples.
        create_pak("${Urho3D_SOURCE_DIR}/bin/Data"               "${CMAKE_BINARY_DIR}/bin/Data.pak")
        create_pak("${Urho3D_SOURCE_DIR}/bin/CoreData"           "${CMAKE_BINARY_DIR}/bin/CoreData.pak")
        create_pak("${Urho3D_SOURCE_DIR}/bin/Autoload/LargeData" "${CMAKE_BINARY_DIR}/bin/Autoload/LargeData.pak")
        package_resources_web(
            FILES        "${CMAKE_BINARY_DIR}/bin/Data.pak"
                         "${CMAKE_BINARY_DIR}/bin/CoreData.pak"
                         "${CMAKE_BINARY_DIR}/bin/Autoload/LargeData.pak"
            RELATIVE_DIR "${CMAKE_BINARY_DIR}/bin"
            OUTPUT       "Resources.js"
        )
    endif ()
endif ()
