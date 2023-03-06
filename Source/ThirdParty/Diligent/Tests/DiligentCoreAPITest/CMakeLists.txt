cmake_minimum_required (VERSION 3.17)

project(DiligentCoreAPITest)

file(GLOB COMMON_SOURCE LIST_DIRECTORIES false src/* src/ObjectCreationFailure/*)
file(GLOB COMMON_INCLUDE LIST_DIRECTORIES false include/*)
file(GLOB INLINE_SHADERS LIST_DIRECTORIES false include/InlineShaders/*)
file(GLOB_RECURSE SHADERS LIST_DIRECTORIES false assets/shaders/*.*)
file(GLOB C_INTERFACE_SOURCE LIST_DIRECTORIES false src/c_interface/*)

set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")

set(SOURCE ${COMMON_SOURCE} ${C_INTERFACE_SOURCE})
set(INCLUDE ${COMMON_INCLUDE})

if(NOT TARGET Diligent-HLSL2GLSLConverterLib)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/HLSL2GLSLConverterTest.cpp)
endif()

if(NOT D3D12_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/DXCompilerTest.cpp)
endif()

if(NOT D3D12_SUPPORTED AND NOT D3D12_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/DXBCUtilsTest.cpp)
endif()

if(NOT ARCHIVER_SUPPORTED)
    list(REMOVE_ITEM SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/ArchiveTest.cpp)
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
    
    set_property(SOURCE ${MTL_SOURCE} APPEND_STRING PROPERTY
        # Disable Metal API deprecation warnings such as
        #   'MTLArgumentType' is deprecated: first deprecated in macOS 13.0
        COMPILE_FLAGS " -Wno-deprecated-declarations"
    )
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    file(GLOB GL_SOURCE LIST_DIRECTORIES false src/GL/*)
    file(GLOB GL_INCLUDE LIST_DIRECTORIES false include/GL/*)
    list(APPEND INCLUDE ${GL_INCLUDE})
    list(APPEND SOURCE ${GL_SOURCE})
endif()

set(ALL_SOURCE ${SOURCE} ${INCLUDE} ${SHADERS} ${INLINE_SHADERS})
add_executable(DiligentCoreAPITest ${ALL_SOURCE})
set_common_target_properties(DiligentCoreAPITest)

get_supported_backends(ENGINE_LIBRARIES)

target_link_libraries(DiligentCoreAPITest
PRIVATE
    Diligent-BuildSettings
    Diligent-TargetPlatform
    Diligent-GPUTestFramework
    Diligent-GraphicsAccessories
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-ShaderTools
)

if(TARGET Diligent-HLSL2GLSLConverterLib)
    target_link_libraries(DiligentCoreAPITest PRIVATE Diligent-HLSL2GLSLConverterLib)
endif()

if(VULKAN_SUPPORTED)
    if(PLATFORM_MACOS)
        if(VULKAN_LIB_PATH)
            # Configure rpath so that the executable can find vulkan library
            set_target_properties(DiligentCoreAPITest PROPERTIES
                BUILD_RPATH "${VULKAN_LIB_PATH}"
            )
        else()
            message(WARNING "Vulkan lib path is not set. API test will fail to start in Vulkan mode")
        endif()
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Disable warnings like this one:
    #   comparison of function 'glPolygonMode' not equal to a null pointer is always true [-Wtautological-pointer-compare]
    set_source_files_properties(
        src/GL/DrawCommandReferenceGL.cpp
        src/GL/GeometryShaderReferenceGL.cpp
        src/GL/TessellationReferenceGL.cpp
    PROPERTIES
        COMPILE_FLAGS -Wno-tautological-pointer-compare
    )
    if (PLATFORM_EMSCRIPTEN)
        set_property(SOURCE ${C_INTERFACE_SOURCE} APPEND_STRING PROPERTY COMPILE_FLAGS "-Wno-unknown-warning-option -Wno-unused-but-set-variable")
    endif()
endif()

target_include_directories(DiligentCoreAPITest
PRIVATE
    include
)

if(EXISTS "${DILIGENT_NVAPI_PATH}/nvapi.h")
    target_include_directories(DiligentCoreAPITest PRIVATE ${DILIGENT_NVAPI_PATH})
    target_link_libraries(DiligentCoreAPITest PRIVATE ${DILIGENT_NVAPI_LIB_PATH})
    target_compile_definitions(DiligentCoreAPITest PUBLIC DILIGENT_ENABLE_D3D_NVAPI)
endif()


set_target_properties(DiligentCoreAPITest
PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

if(PLATFORM_WIN32)
    copy_required_dlls(DiligentCoreAPITest)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ALL_SOURCE})

set_target_properties(DiligentCoreAPITest PROPERTIES
    FOLDER "DiligentCore/Tests"
)
