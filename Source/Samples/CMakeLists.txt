#
# Copyright (c) 2008-2020 the Urho3D project.
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

if (NOT URHO3D_SAMPLES)
    return ()
endif ()

if (NOT URHO3D_WIN32_CONSOLE)
    set (PLATFORM WIN32)
endif ()

## Add samples
file (GLOB_RECURSE DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CMakeLists.txt)
list (SORT DIRS)
foreach (DIR ${DIRS})
    get_filename_component (DIR ${DIR} PATH)
    if (DIR AND NOT TARGET ${DIR})
        add_subdirectory (${DIR})
    endif ()
endforeach ()

file (GLOB SAMPLE_MANAGER_SOURCE_CODE *.h *.cpp)

if (ANDROID)
    add_library(Samples MODULE ${SAMPLE_MANAGER_SOURCE_CODE} ${SOURCE_CODE})
else ()
    add_executable(Samples ${PLATFORM} MACOSX_BUNDLE ${SAMPLE_MANAGER_SOURCE_CODE} ${SOURCE_CODE})
endif ()
target_link_libraries(Samples PUBLIC Urho3D)
target_include_directories(Samples PRIVATE .)
set_property(TARGET Samples PROPERTY FOLDER "Sample Projects")
if (TARGET Utilities2D)
    target_link_libraries(Samples PUBLIC Utilities2D)
endif ()

vs_group_subdirectory_targets(${CMAKE_CURRENT_SOURCE_DIR} "Sample Projects")

# Package resources for samples.
if (URHO3D_PACKAGING)
    create_pak("${rbfx_SOURCE_DIR}/bin/Data"               "${CMAKE_BINARY_DIR}/bin/Data.pak")
    create_pak("${rbfx_SOURCE_DIR}/bin/CoreData"           "${CMAKE_BINARY_DIR}/bin/CoreData.pak")
    create_pak("${rbfx_SOURCE_DIR}/bin/Autoload/LargeData" "${CMAKE_BINARY_DIR}/bin/Autoload/LargeData.pak")
    set (URHO3D_DATA_PAKS "${CMAKE_BINARY_DIR}/bin/Data.pak" "${CMAKE_BINARY_DIR}/bin/CoreData.pak")
    set (URHO3D_AUTOLOAD_PAKS "${CMAKE_BINARY_DIR}/bin/Autoload/LargeData.pak")
endif ()

if (WEB)
    web_executable(Samples)
    package_resources_web(
        FILES        "${CMAKE_BINARY_DIR}/bin/Data.pak"
                     "${CMAKE_BINARY_DIR}/bin/CoreData.pak"
                     "${CMAKE_BINARY_DIR}/bin/Autoload/LargeData.pak"
        RELATIVE_DIR "${CMAKE_BINARY_DIR}/bin"
        OUTPUT       "Resources.js"
        INSTALL_TO   ${DEST_BIN_DIR_CONFIG}
    )
    web_link_resources(Samples Resources.js)
    target_link_libraries(Samples PRIVATE "--shell-file ${CMAKE_SOURCE_DIR}/bin/shell.html")
    install(FILES "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Samples.js" DESTINATION ${DEST_BIN_DIR_CONFIG})
    if (EMSCRIPTEN_WASM)
        install(FILES "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Samples.wasm" DESTINATION ${DEST_BIN_DIR_CONFIG})
    endif ()
endif ()

if (XCODE)
    # Default app bundle icon
    set(RESOURCE_FILES ${rbfx_SOURCE_DIR}/bin/Data/Textures/UrhoIcon.icns)
    if (IOS)
        # Default app icon on the iOS/tvOS home screen
        list(APPEND RESOURCE_FILES ${rbfx_SOURCE_DIR}/bin/Data/Textures/UrhoIcon.png)
    endif ()
    set_source_files_properties (${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(Samples PRIVATE ${RESOURCE_FILES})

    if (IOS)
        set_target_properties(Samples
            PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME \${PRODUCT_NAME}
                       MACOSX_BUNDLE_GUI_IDENTIFIER com.github.rbfx.\${PRODUCT_NAME:rfc1034identifier:lower}
        )
        if (URHO3D_PACKAGING)
            set_source_files_properties (${URHO3D_DATA_PAKS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            set_source_files_properties (${URHO3D_AUTOLOAD_PAKS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/Autoload)
            target_sources(Samples PRIVATE ${URHO3D_DATA_PAKS} ${URHO3D_AUTOLOAD_PAKS})
        else ()
            set_source_files_properties (
                "${rbfx_SOURCE_DIR}/bin/Data" "${rbfx_SOURCE_DIR}/bin/CoreData" "${rbfx_SOURCE_DIR}/bin/Autoload"
                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
            target_sources(Samples PRIVATE
                "${rbfx_SOURCE_DIR}/bin/Data" "${rbfx_SOURCE_DIR}/bin/CoreData" "${rbfx_SOURCE_DIR}/bin/Autoload")
        endif ()
    endif ()
endif ()

if (MSVC)
    if (URHO3D_PACKAGING)
        # Include .pak files under "Resource Files" in VS project.
        source_group("Resource Files" FILES ${URHO3D_DATA_PAKS} ${URHO3D_AUTOLOAD_PAKS})
        # Ensure file VS wont try to build resources.
        set_property(SOURCE ${URHO3D_DATA_PAKS} ${URHO3D_AUTOLOAD_PAKS} PROPERTY VS_TOOL_OVERRIDE None)
        # Include .pak files in UWP app. VS_DEPLOYMENT_LOCATION is not set, they will go into root of app dir.
        set_property(SOURCE ${URHO3D_DATA_PAKS} ${URHO3D_AUTOLOAD_PAKS} PROPERTY VS_DEPLOYMENT_CONTENT 1)
        # Consume file. Without this resources are just ignored.
        target_sources(Samples PRIVATE ${URHO3D_DATA_PAKS} ${URHO3D_AUTOLOAD_PAKS})
    else ()
        foreach (subfolder Data CoreData Autoload)
            file(GLOB_RECURSE DATA_FILES RELATIVE "${rbfx_SOURCE_DIR}/bin" "${rbfx_SOURCE_DIR}/bin/${subfolder}/**")
            foreach (filename ${DATA_FILES})
                set (filepath "${rbfx_SOURCE_DIR}/bin/${filename}")
                get_filename_component(dirname "${filename}" DIRECTORY)
                # Include file in UWP app.
                set_property(SOURCE ${filepath} PROPERTY VS_DEPLOYMENT_CONTENT 1)
                # Set file path by which UWP app will access it.
                set_property(SOURCE ${filepath} PROPERTY VS_DEPLOYMENT_LOCATION "${dirname}")
                # Ensure file VS wont try to build resources.
                set_property(SOURCE ${filepath} PROPERTY VS_TOOL_OVERRIDE None)
                # Include file under "Resource Files" in VS project.
                source_group("Resource Files/${dirname}" FILES "${filepath}")
                # Consume file. Without this resources are just ignored.
                target_sources(Samples PRIVATE "${filepath}")
            endforeach ()
        endforeach ()
    endif ()
endif ()

if (NOT ANDROID)
    install(TARGETS Samples
        RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
        LIBRARY DESTINATION ${DEST_BIN_DIR_CONFIG}
        BUNDLE DESTINATION  ${DEST_BIN_DIR_CONFIG}
    )
endif ()
