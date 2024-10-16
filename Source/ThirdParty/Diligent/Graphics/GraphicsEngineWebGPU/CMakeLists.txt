cmake_minimum_required (VERSION 3.10)

project(Diligent-GraphicsEngineWebGPU CXX)

set(INCLUDE
    include/AttachmentCleanerWebGPU.hpp
    include/BufferViewWebGPUImpl.hpp
    include/BufferWebGPUImpl.hpp
    include/DearchiverWebGPUImpl.hpp
    include/DeviceContextWebGPUImpl.hpp
    include/DeviceObjectArchiveWebGPU.hpp
    include/DynamicMemoryManagerWebGPU.hpp
    include/UploadMemoryManagerWebGPU.hpp
    include/EngineWebGPUImplTraits.hpp
    include/FenceWebGPUImpl.hpp
    include/FramebufferWebGPUImpl.hpp
    include/GenerateMipsHelperWebGPU.hpp
    include/pch.h
    include/PipelineResourceAttribsWebGPU.hpp
    include/PipelineResourceSignatureWebGPUImpl.hpp
    include/PipelineStateWebGPUImpl.hpp
    include/PipelineLayoutWebGPU.hpp
    include/QueryManagerWebGPU.hpp
    include/QueryWebGPUImpl.hpp
    include/RenderDeviceWebGPUImpl.hpp
    include/RenderPassWebGPUImpl.hpp
    include/SamplerWebGPUImpl.hpp
    include/ShaderResourceBindingWebGPUImpl.hpp
    include/ShaderResourceCacheWebGPU.hpp
    include/ShaderVariableManagerWebGPU.hpp
    include/ShaderWebGPUImpl.hpp
    include/SwapChainWebGPUImpl.hpp
    include/SyncPointWebGPU.hpp
    include/TextureViewWebGPUImpl.hpp
    include/TextureWebGPUImpl.hpp
    include/WebGPUObjectWrappers.hpp
    include/WebGPUTypeConversions.hpp
    include/WebGPUResourceBase.hpp
    include/WebGPUStubs.hpp
)

set(INTERFACE 
   interface/BufferViewWebGPU.h
   interface/BufferWebGPU.h
   interface/DeviceContextWebGPU.h
   interface/EngineFactoryWebGPU.h
   interface/FenceWebGPU.h
   interface/PipelineStateWebGPU.h
   interface/QueryWebGPU.h
   interface/RenderDeviceWebGPU.h
   interface/SamplerWebGPU.h
   interface/ShaderResourceBindingWebGPU.h
   interface/ShaderWebGPU.h
   interface/SwapChainWebGPU.h
   interface/TextureViewWebGPU.h
   interface/TextureWebGPU.h
)

set(SRC
   src/AttachmentCleanerWebGPU.cpp
   src/BufferViewWebGPUImpl.cpp
   src/BufferWebGPUImpl.cpp
   src/DearchiverWebGPUImpl.cpp
   src/DeviceContextWebGPUImpl.cpp
   src/DeviceObjectArchiveWebGPU.cpp
   src/DynamicMemoryManagerWebGPU.cpp
   src/UploadMemoryManagerWebGPU.cpp
   src/EngineFactoryWebGPU.cpp
   src/FenceWebGPUImpl.cpp
   src/FramebufferWebGPUImpl.cpp
   src/GenerateMipsHelperWebGPU.cpp
   src/PipelineResourceSignatureWebGPUImpl.cpp
   src/PipelineStateWebGPUImpl.cpp
   src/PipelineLayoutWebGPU.cpp
   src/QueryManagerWebGPU.cpp
   src/QueryWebGPUImpl.cpp
   src/RenderDeviceWebGPUImpl.cpp
   src/RenderPassWebGPUImpl.cpp
   src/SamplerWebGPUImpl.cpp
   src/ShaderResourceBindingWebGPUImpl.cpp
   src/ShaderResourceCacheWebGPU.cpp
   src/ShaderVariableManagerWebGPU.cpp
   src/ShaderWebGPUImpl.cpp
   src/SwapChainWebGPUImpl.cpp
   src/TextureViewWebGPUImpl.cpp
   src/TextureWebGPUImpl.cpp
   src/WebGPUTypeConversions.cpp
   src/WebGPUResourceBase.cpp
)

set(DLL_SOURCE
    src/DLLMain.cpp
    src/GraphicsEngineWebGPU.def
)

add_library(Diligent-GraphicsEngineWebGPUInterface INTERFACE)
target_link_libraries     (Diligent-GraphicsEngineWebGPUInterface INTERFACE Diligent-GraphicsEngineInterface)
target_include_directories(Diligent-GraphicsEngineWebGPUInterface INTERFACE interface)

add_library(Diligent-GraphicsEngineWebGPU-static STATIC 
    ${SRC} ${INTERFACE} ${INCLUDE}
    readme.md
)

add_library(Diligent-GraphicsEngineWebGPU-shared SHARED 
    readme.md
)

if(MSVC)
    target_sources(Diligent-GraphicsEngineWebGPU-shared PRIVATE ${DLL_SOURCE})
endif()

target_include_directories(Diligent-GraphicsEngineWebGPU-static 
PRIVATE
    include
)

set(PRIVATE_DEPENDENCIES
    Diligent-BuildSettings
    Diligent-BuildSettings
    Diligent-TargetPlatform
    Diligent-Common
    Diligent-GraphicsEngine
    Diligent-ShaderTools
)

set(PUBLIC_DEPENDENCIES
   Diligent-GraphicsEngineWebGPUInterface
)

if (${DILIGENT_NO_HLSL})
    message("HLSL support is disabled. WebGPU backend will not be able to consume SPIRV bytecode generated from HLSL")
else()
    list(APPEND PRIVATE_DEPENDENCIES SPIRV-Tools-opt)
endif()

if (NOT PLATFORM_EMSCRIPTEN)
    list(APPEND PRIVATE_DEPENDENCIES webgpu_dawn)
endif()

target_link_libraries(Diligent-GraphicsEngineWebGPU-static
PRIVATE
    ${PRIVATE_DEPENDENCIES}
PUBLIC
    ${PUBLIC_DEPENDENCIES}
)

target_link_libraries(Diligent-GraphicsEngineWebGPU-shared
PRIVATE
    Diligent-BuildSettings 
    Diligent-GraphicsEngineWebGPU-static
PUBLIC
    Diligent-GraphicsEngineWebGPUInterface
)

if(PLATFORM_WIN32)
    # Do not add 'lib' prefix when building with MinGW
    set_target_properties(Diligent-GraphicsEngineWebGPU-shared PROPERTIES PREFIX "")

    # Set output name to GraphicsEngineWebGPU_{32|64}{r|d}
    set_dll_output_name(Diligent-GraphicsEngineWebGPU-shared GraphicsEngineWebGPU)
else()
    set_target_properties(Diligent-GraphicsEngineWebGPU-shared PROPERTIES
        OUTPUT_NAME Diligent-GraphicsEngineWebGPU
    )
endif()

set_common_target_properties(Diligent-GraphicsEngineWebGPU-shared 17)
set_common_target_properties(Diligent-GraphicsEngineWebGPU-static 17)

if (${DILIGENT_NO_GLSLANG})
    message("GLSLang is not being built. WebGPU backend will only be able to consume SPIRV byte code.")
endif()

target_compile_definitions(Diligent-GraphicsEngineWebGPU-static
PRIVATE
    DILIGENT_NO_GLSLANG=$<BOOL:${DILIGENT_NO_GLSLANG}>
    DILIGENT_NO_HLSL=$<BOOL:${DILIGENT_NO_HLSL}>
)

if (PLATFORM_EMSCRIPTEN)
    target_link_options(Diligent-GraphicsEngineWebGPU-static PUBLIC "SHELL: -s USE_WEBGPU=1 -s USE_PTHREADS=1")
endif()

target_compile_definitions(Diligent-GraphicsEngineWebGPU-shared PUBLIC ENGINE_DLL=1)

if (MSVC)
    target_compile_options(Diligent-GraphicsEngineWebGPU-static PRIVATE /wd4324)
endif()

source_group("src" FILES ${SRC})
if(PLATFORM_WIN32)
    source_group("dll" FILES ${DLL_SOURCE})
endif()

source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngineWebGPU-static PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GraphicsEngineWebGPU-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineWebGPU-shared)
    install_core_lib(Diligent-GraphicsEngineWebGPU-static)
endif()
