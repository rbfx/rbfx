cmake_minimum_required (VERSION 3.6)

project(DiligentCore-IncludeTest)

file(GLOB_RECURSE SOURCE LIST_DIRECTORIES false *.cpp *.c *.mm *.m)

if(NOT D3D11_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D11_INC_TEST LIST_DIRECTORIES false GraphicsEngineD3D11/*.cpp GraphicsEngineD3D11/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_D3D11_INC_TEST})
endif()

if(NOT D3D12_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3D12_INC_TEST LIST_DIRECTORIES false GraphicsEngineD3D12/*.cpp GraphicsEngineD3D12/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_D3D12_INC_TEST})
endif()

if(NOT D3D11_SUPPORTED AND NOT D3D12_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_D3DBASE_INC_TEST LIST_DIRECTORIES false GraphicsEngineD3DBase/*.cpp GraphicsEngineD3DBase/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_D3DBASE_INC_TEST})
endif()

if(NOT GL_SUPPORTED AND NOT GLES_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_GL_INC_TEST LIST_DIRECTORIES false GraphicsEngineOpenGL/*.cpp GraphicsEngineOpenGL/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_GL_INC_TEST})
endif()

if(NOT VULKAN_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_VK_INC_TEST LIST_DIRECTORIES false GraphicsEngineVk/*.cpp GraphicsEngineVk/*.c)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_VK_INC_TEST})
endif()

if(NOT METAL_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_MTL_INC_TEST LIST_DIRECTORIES false GraphicsEngineMetal/*.mm GraphicsEngineMetal/*.m)
    list(REMOVE_ITEM SOURCE ${GRAPHICS_ENGINE_MTL_INC_TEST})
endif()

if(NOT PLATFORM_WIN32 AND NOT PLATFORM_UNIVERSAL_WINDOWS)
    list(REMOVE_ITEM SOURCE
         ${CMAKE_CURRENT_SOURCE_DIR}/Common/CommonH_Wnd_test.cpp
         ${CMAKE_CURRENT_SOURCE_DIR}/GraphicsAccessories/GraphicsAccessoriesH_Wnd_test.cpp
         )
endif()

# TODO: remove if vulkan_core.h is fixed (https://github.com/KhronosGroup/Vulkan-Docs/issues/1769)
if(VULKAN_SUPPORTED)
    file(GLOB GRAPHICS_ENGINE_VK_INC_TEST LIST_DIRECTORIES false GraphicsEngineVk/*.cpp GraphicsEngineVk/*.c)
    set_property(SOURCE ${GRAPHICS_ENGINE_VK_INC_TEST} APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/Vulkan-Headers/include")
endif()

add_library(DiligentCore-IncludeTest ${SOURCE})

target_include_directories(DiligentCore-IncludeTest PRIVATE ../../..)
target_link_libraries(DiligentCore-IncludeTest PRIVATE Diligent-BuildSettings)
set_common_target_properties(DiligentCore-IncludeTest)

if(MSVC)
    set_target_properties(DiligentCore-IncludeTest PROPERTIES
        STATIC_LIBRARY_FLAGS "/IGNORE:4221"
    )

    # Treat warnings as errors to catch c compilation issues
    target_compile_options(DiligentCore-IncludeTest PRIVATE "/WX")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (PLATFORM_EMSCRIPTEN)
        set_source_files_properties(
            Primitives/ReferenceCountersH_test.c
            PROPERTIES 
            COMPILE_FLAGS "-Wno-unknown-warning-option -Wno-unused-but-set-variable"
        )
    endif()
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE})

set_target_properties(DiligentCore-IncludeTest PROPERTIES
    FOLDER "DiligentCore/Tests"
)
