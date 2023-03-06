cmake_minimum_required (VERSION 3.6)

project(Diligent-EmscriptenPlatform CXX)

set(INTERFACE
    interface/EmscriptenDebug.hpp
    interface/EmscriptenFileSystem.hpp
    interface/EmscriptenPlatformDefinitions.h
    interface/EmscriptenPlatformMisc.hpp
    interface/EmscriptenNativeWindow.h
    ../Linux/interface/LinuxPlatformMisc.hpp
)

set(SOURCE
    src/EmscriptenDebug.cpp
    src/EmscriptenFileSystem.cpp
)

add_library(Diligent-EmscriptenPlatform ${SOURCE} ${INTERFACE} ${PLATFORM_INTERFACE_HEADERS})
set_common_target_properties(Diligent-EmscriptenPlatform)

target_include_directories(Diligent-EmscriptenPlatform
PUBLIC
    interface
)

target_link_libraries(Diligent-EmscriptenPlatform
PRIVATE
    Diligent-BuildSettings
PUBLIC
    Diligent-BasicPlatform
    Diligent-PlatformInterface
)

source_group("src" FILES ${SOURCE})
source_group("interface\\emscripten" FILES ${INTERFACE})
source_group("interface\\common" FILES ${PLATFORM_INTERFACE_HEADERS})

set_target_properties(Diligent-EmscriptenPlatform PROPERTIES
    FOLDER DiligentCore/Platforms
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-EmscriptenPlatform)

    get_target_relative_dir(Diligent-EmscriptenPlatform RELATIVE_PATH)
    install(DIRECTORY    ../Linux/interface
            DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}/${RELATIVE_PATH}/../Linux"
    )
endif()
