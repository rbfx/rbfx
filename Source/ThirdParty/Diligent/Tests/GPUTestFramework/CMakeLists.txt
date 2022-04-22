cmake_minimum_required (VERSION 3.17)

project(Diligent-GPUTestFramework)

set(SOURCE src/GPUTestingEnvironment.cpp src/TestingSwapChainBase.cpp)
set(INCLUDE include/GPUTestingEnvironment.hpp include/TestingSwapChainBase.hpp)

if(PLATFORM_WIN32)
    file(GLOB SOURCE_WIN32 LIST_DIRECTORIES false src/Win32/*)
    list(APPEND SOURCE ${SOURCE_WIN32})
elseif(PLATFORM_MACOS)
    file(GLOB SOURCE_MACOS LIST_DIRECTORIES false src/MacOS/*)
    list(APPEND SOURCE ${SOURCE_MACOS})
elseif(PLATFORM_LINUX)
    file(GLOB SOURCE_LINUX LIST_DIRECTORIES false src/Linux/*)
    list(APPEND SOURCE ${SOURCE_LINUX})
elseif(PLATFORM_EMSCRIPTEN)
    file(GLOB SOURCE_EMSCRIPTEN LIST_DIRECTORIES false src/Emscripten/*)
    list(APPEND SOURCE ${SOURCE_EMSCRIPTEN})
endif()

if(D3D11_SUPPORTED)
    file(GLOB D3D11_SOURCE LIST_DIRECTORIES false src/D3D11/*)
    file(GLOB D3D11_INCLUDE LIST_DIRECTORIES false include/D3D11/*)
    list(APPEND INCLUDE ${D3D11_INCLUDE})
    list(APPEND SOURCE ${D3D11_SOURCE})
endif()

if(D3D12_SUPPORTED)
    file(GLOB D3D12_SOURCE LIST_DIRECTORIES false src/D3D12/*)
    file(GLOB D3D12_INCLUDE LIST_DIRECTORIES false include/D3D12/*)
    list(APPEND INCLUDE ${D3D12_INCLUDE})
    list(APPEND SOURCE ${D3D12_SOURCE})
endif()

if(VULKAN_SUPPORTED)
    file(GLOB VK_SOURCE LIST_DIRECTORIES false src/Vulkan/*)
    file(GLOB VK_INCLUDE LIST_DIRECTORIES false include/Vulkan/*)
    list(APPEND INCLUDE ${VK_INCLUDE})
    list(APPEND SOURCE ${VK_SOURCE})
endif()

if(METAL_SUPPORTED)
    file(GLOB MTL_SOURCE LIST_DIRECTORIES false src/Metal/*)
    file(GLOB MTL_INCLUDE LIST_DIRECTORIES false include/Metal/*)
    list(APPEND INCLUDE ${MTL_INCLUDE})
    list(APPEND SOURCE ${MTL_SOURCE})
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    file(GLOB GL_SOURCE LIST_DIRECTORIES false src/GL/*)
    file(GLOB GL_INCLUDE LIST_DIRECTORIES false include/GL/*)
    list(APPEND INCLUDE ${GL_INCLUDE})
    list(APPEND SOURCE ${GL_SOURCE})
endif()

set(ALL_SOURCE ${SOURCE} ${INCLUDE})
add_library(Diligent-GPUTestFramework STATIC ${ALL_SOURCE})
set_common_target_properties(Diligent-GPUTestFramework)

get_supported_backends(ENGINE_LIBRARIES)

target_link_libraries(Diligent-GPUTestFramework
PRIVATE
    Diligent-BuildSettings
    Diligent-Common
    Diligent-ShaderTools
PUBLIC
    ${ENGINE_LIBRARIES}
    Diligent-TestFramework
    Diligent-GraphicsAccessories
)

if(D3D11_SUPPORTED OR D3D12_SUPPORTED)
    target_link_libraries(Diligent-GPUTestFramework PUBLIC d3dcompiler.lib)
endif()

if(D3D12_SUPPORTED)
    target_link_libraries(Diligent-GPUTestFramework PUBLIC d3d12.lib)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    if(PLATFORM_WIN32)
        target_link_libraries(Diligent-GPUTestFramework PUBLIC GLEW::glew opengl32.lib)
    elseif(PLATFORM_LINUX)
        target_link_libraries(Diligent-GPUTestFramework PUBLIC GLEW::glew)
    elseif(PLATFORM_MACOS)
        find_package(OpenGL REQUIRED)
        target_link_libraries(Diligent-GPUTestFramework PUBLIC GLEW::glew ${OPENGL_LIBRARY})
    elseif(PLATFORM_EMSCRIPTEN)

    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif()

if(PLATFORM_LINUX)
    target_link_libraries(Diligent-GPUTestFramework PUBLIC GL X11)
endif()

if(VULKAN_SUPPORTED)
    target_link_libraries(Diligent-GPUTestFramework PUBLIC Vulkan::Headers volk::volk_headers)

    if(PLATFORM_LINUX)
        target_link_libraries(Diligent-GPUTestFramework
        PUBLIC
            xcb
        )
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Disable the following warning:
    #   comparison of function 'glPolygonMode' not equal to a null pointer is always true [-Wtautological-pointer-compare]
    set_property(SOURCE
        src/GL/TestingEnvironmentGL.cpp
    APPEND_STRING PROPERTY
        COMPILE_FLAGS -Wno-tautological-pointer-compare
    )
    if (PLATFORM_EMSCRIPTEN)
        set_property(SOURCE
            src/GPUTestingEnvironment.cpp
        APPEND_STRING PROPERTY
            COMPILE_FLAGS "-Wno-unused-variable"
        )
    endif()
endif()

target_include_directories(Diligent-GPUTestFramework
PUBLIC
    include
)

target_compile_definitions(Diligent-GPUTestFramework PUBLIC DILIGENT_NO_GLSLANG=$<BOOL:${DILIGENT_NO_GLSLANG}>)
if(${D3D12_H_HAS_MESH_SHADER})
    target_compile_definitions(Diligent-GPUTestFramework PUBLIC D3D12_H_HAS_MESH_SHADER)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE})

set_target_properties(Diligent-GPUTestFramework PROPERTIES
    FOLDER "DiligentCore/Tests"
)

if (TARGET Diligent-Archiver-shared)
    target_link_libraries(Diligent-GPUTestFramework PUBLIC Diligent-Archiver-shared)
endif()
