# Copyright (c) 2008-2020 the Urho3D project.
# Copyright (c) 2020-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# Enable parallel builds in Visual Studio
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

# Silence annoying warnings
add_compile_definitions($<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
# Silence C++23 deprecated literal operator warnings
if (CMAKE_CXX_STANDARD GREATER_EQUAL 23 AND (CLANG OR GCC))
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-literal-operator>)
endif()

# Package is found in the main CMakeLists.txt
if (Urho3DThirdParty_FOUND)
    message(STATUS "Using external ThirdParty: ${Urho3DThirdParty_INCLUDE_DIRS}")
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

install(FILES
    ${rbfx_SOURCE_DIR}/script/rbfx.natvis
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
install(DIRECTORY
    ${rbfx_SOURCE_DIR}/CMake/Toolchains/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/Urho3D/CMake/Toolchains
    FILES_MATCHING PATTERN "*.cmake"
)
