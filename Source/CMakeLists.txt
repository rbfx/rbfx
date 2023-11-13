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

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # Enable parallel builds
    if (URHO3D_PARALLEL_BUILD GREATER_EQUAL 2)
        ucm_add_flags(/MP${URHO3D_PARALLEL_BUILD})
    elseif (URHO3D_PARALLEL_BUILD)
        cmake_host_system_information(RESULT URHO3D_PARALLEL_BUILD QUERY NUMBER_OF_PHYSICAL_CORES)
        ucm_add_flags(/MP${URHO3D_PARALLEL_BUILD})
    endif ()
endif ()

add_subdirectory (ThirdParty)
add_subdirectory (Urho3D)
add_subdirectory (Tools)
add_subdirectory (Samples)
add_subdirectory (Tests)

# Check options outside so user can add Player and/or Editor explicitly afterwards.
if (URHO3D_PLAYER)
    add_subdirectory (Player)
endif ()
if (URHO3D_EDITOR)
    add_subdirectory (Editor)
endif ()

# Generate a helper script for consuming built tools during crosscompiling.
set(SWIG_EXECUTABLE OFF)
set(PACKAGE_TOOL OFF)
if (TARGET swig)
    set (SWIG_EXECUTABLE ${DEST_BIN_DIR_CONFIG}/$<TARGET_FILE_NAME:swig>)
endif ()
if (TARGET PackageTool)
    set (PACKAGE_TOOL ${DEST_BIN_DIR_CONFIG}/$<TARGET_FILE_NAME:PackageTool>)
endif ()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/share/CMake)
configure_file("${rbfx_SOURCE_DIR}/CMake/SDKTools.cmake.in" "${CMAKE_BINARY_DIR}/share/CMake/SDKTools.cmake.in" @ONLY)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/share/CMake/SDKTools-$<CONFIG>.cmake" INPUT "${CMAKE_BINARY_DIR}/share/CMake/SDKTools.cmake.in")
file(COPY "${rbfx_SOURCE_DIR}/CMake/SDKTools.cmake" DESTINATION "${CMAKE_BINARY_DIR}/share/CMake/")
foreach (config ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    install(FILES ${CMAKE_BINARY_DIR}/share/CMake/SDKTools-${config}.cmake DESTINATION ${DEST_SHARE_DIR}/CMake)
endforeach ()
install(FILES ${CMAKE_BINARY_DIR}/share/CMake/SDKTools.cmake DESTINATION ${DEST_SHARE_DIR}/CMake)
unset(config)

if (URHO3D_CSHARP AND NOT CMAKE_VS_MSBUILD_COMMAND)
    # Generate a VS solution for C# projects when outside of visual studio.
    vs_generate_sln(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln ${RBFX_CSPROJ_LIST})

    if (LINUX)
        # On linux instance of dotnet may hang indefinitely which prevents CMake from detecting build completion and hangs the build.
        set (DOTNET_LINUX_MSBUILD_WORKAROUND COMMAND ${rbfx_SOURCE_DIR}/script/.kill_dangling_dotnet.sh ${DOTNET})
    elseif (MACOS)
        # On MacOS Xcode pollutes environment which causes MSBuild to compile all managed targets into Urho3DNet assembly and compilation fails because file contents is not what next build step expects.
        set (DOTNET_MACOS_MSBUILD_WORKAROUND env -u TARGET_NAME -u TARGETNAME)
    endif ()
    add_custom_target(BuildManagedTargets ALL
        COMMAND ${DOTNET_MACOS_MSBUILD_WORKAROUND} ${DOTNET} build ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln
            -restore -noLogo -interactive:false -nr:false -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/
            -consoleLoggerParameters:ErrorsOnly -p:Platform=${URHO3D_PLATFORM} -p:Configuration=${URHO3D_CONFIG}
            ${DOTNET_LINUX_MSBUILD_WORKAROUND}
        DEPENDS Urho3D)
    foreach (TARGET libEditorWrapper libPlayer)
        if (TARGET ${TARGET})
            add_dependencies(BuildManagedTargets ${TARGET})
        endif ()
    endforeach ()
endif ()
install(FILES
    ${rbfx_SOURCE_DIR}/script/rbfx.natvis
    ${rbfx_SOURCE_DIR}/script/rbfx.vscode.unix.natvis
    DESTINATION ${DEST_SHARE_DIR})

# This is a normal install that users are supposed to use.
install(EXPORT Urho3D DESTINATION ${DEST_SHARE_DIR}/CMake)
