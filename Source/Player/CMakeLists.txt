# Copyright (c) 2017-2022 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# Always export Player without entry point as PlayerLibrary.
add_library (PlayerLibrary STATIC PlayerApplication.cpp PlayerApplication.h)
target_include_directories(PlayerLibrary PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Tools>
)
target_link_libraries(PlayerLibrary PUBLIC Urho3D)

if (NOT URHO3D_NO_EDITOR_PLAYER_EXE)
    if (ANDROID OR URHO3D_CSHARP)
        add_library (libPlayer MODULE ${SOURCE_FILES} Player.cpp)
        set_target_properties(libPlayer PROPERTIES PREFIX "")
        set (PLAYER_TARGET libPlayer)
        if (URHO3D_CSHARP)
            add_target_csharp(
                TARGET Player
                EXE
                PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/ManagedHost/Player.csproj
                OUTPUT ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/Player
                DEPENDS Urho3DNet)
            if (TARGET Player)
                add_dependencies(Player libPlayer Urho3DNet)
            endif ()
            install(FILES "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/Player${CMAKE_EXECUTABLE_SUFFIX}" DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif ()
    elseif (NOT WEB)
        # Player executable is not usefil in web builds, because there would be no way to load user code.
        if (NOT URHO3D_WIN32_CONSOLE)
            set (TARGET_TYPE WIN32)
        endif ()
        add_executable (Player ${TARGET_TYPE} Player.cpp)
        set (PLAYER_TARGET Player)
    endif ()

    if (PLAYER_TARGET)
        target_link_libraries(${PLAYER_TARGET} PUBLIC PlayerLibrary)
        target_link_plugins(${PLAYER_TARGET} "${URHO3D_PLUGIN_LIST}")
    endif ()
endif ()

# SDK installation
if (PLAYER_TARGET)
    install (TARGETS ${PLAYER_TARGET}
        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Install runtime dependencies
    install_target_runtime_deps(TARGET ${PLAYER_TARGET})
endif ()

install (TARGETS PlayerLibrary EXPORT Urho3D ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Tools
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
