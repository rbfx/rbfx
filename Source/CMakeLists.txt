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

# Enable parallel builds in Visual Studio
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

# Silence annoying warnings
add_compile_definitions($<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

# Package is found in the main CMakeLists.txt
if (Urho3DThirdParty_FOUND)
    message(STATUS "Using external ThirdParty component")
else ()
    message(STATUS "External ThirdParty not found, building from source")
    add_subdirectory (ThirdParty)
endif ()

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

# Generate CMake config files for tools export
if (DESKTOP)
    # Install the tools export file
    install(EXPORT Urho3DTools
        FILE Urho3DTools.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3DTools/CMake
        COMPONENT Tools
    )

    # Configure and install the tools config file
    # Generate version file to ensure host tools compatibility when cross-compiling.
    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        ${rbfx_SOURCE_DIR}/CMake/Urho3DToolsConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/Urho3DToolsConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3DTools/CMake/
    )

    # Generate the version file for Urho3DTools with host platform compatibility checking
    configure_file(
        ${rbfx_SOURCE_DIR}/CMake/Urho3DToolsConfigVersion.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/Urho3DToolsConfigVersion.cmake
        @ONLY
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/Urho3DToolsConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/Urho3DToolsConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3DTools/CMake/
        COMPONENT Tools
    )
    # Install PlatformTag.cmake module needed by Urho3DToolsConfigVersion.cmake
    install(FILES
        ${rbfx_SOURCE_DIR}/CMake/Modules/PlatformTag.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3DTools/CMake/Modules/
        COMPONENT Tools
    )
endif ()

if (URHO3D_CSHARP AND NOT CMAKE_VS_MSBUILD_COMMAND)
    # Generate a VS solution for C# projects when outside of visual studio.
    vs_generate_sln(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln ${RBFX_CSPROJ_LIST})

    if (LINUX)
        # On linux instance of dotnet may hang indefinitely which prevents CMake from detecting build completion and hangs the build.
        # 2025-11: Issue persists on CI (dotnet 8.0), even though builds seem to succeed locally.
        set (DOTNET_LINUX_MSBUILD_WORKAROUND COMMAND ${rbfx_SOURCE_DIR}/script/.kill_dangling_dotnet.sh ${DOTNET})
    elseif ("${CMAKE_GENERATOR}" MATCHES "^Xcode")
        # On MacOS Xcode pollutes environment which causes MSBuild to compile all managed targets into Urho3DNet assembly and compilation fails because file contents is not what next build step expects.
        set (DOTNET_MACOS_MSBUILD_WORKAROUND env -u TARGET_NAME -u TARGETNAME)
    endif ()
    add_custom_target(BuildManagedTargets ALL
        COMMAND ${DOTNET_MACOS_MSBUILD_WORKAROUND} ${DOTNET} build ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln
            -restore -noLogo -interactive:false -nr:false -p:CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/
            -consoleLoggerParameters:ErrorsOnly -p:Platform=${URHO3D_PLATFORM} -p:Configuration=${URHO3D_CONFIG}
            "-bl:${CMAKE_BINARY_DIR}/ManagedTargets.binlog"
            "-v:diag"
            "-flp:logfile=${CMAKE_BINARY_DIR}/ManagedTargets-log.log"
            ${DOTNET_LINUX_MSBUILD_WORKAROUND}
        DEPENDS Urho3D COMMAND_EXPAND_LISTS)
    foreach (TARGET libEditorWrapper libPlayer)
        if (TARGET ${TARGET})
            add_dependencies(BuildManagedTargets ${TARGET})
        endif ()
    endforeach ()
endif ()
install(FILES
    ${rbfx_SOURCE_DIR}/script/rbfx.natvis
    ${rbfx_SOURCE_DIR}/script/rbfx.vscode.unix.natvis
    DESTINATION ${CMAKE_INSTALL_DATADIR})

# This is a normal install that users are supposed to use.
install (EXPORT Urho3D DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/CMake FILE Urho3D.cmake)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${rbfx_SOURCE_DIR}/CMake/Urho3DConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Urho3DConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/CMake/
)

# Generate the version file for Urho3D with platform compatibility checking
configure_file(
    ${rbfx_SOURCE_DIR}/CMake/Urho3DConfigVersion.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/Urho3DConfigVersion.cmake
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/Urho3DConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/Urho3DConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/CMake/
)