cmake_minimum_required (VERSION 3.6)

project(Diligent-GraphicsEngine CXX)

set(INCLUDE 
    include/BufferBase.hpp
    include/BufferViewBase.hpp
    include/BottomLevelASBase.hpp
    include/CommandListBase.hpp
    include/DearchiverBase.hpp
    include/DefaultShaderSourceStreamFactory.h
    include/Defines.h
    include/DeviceContextBase.hpp
    include/DeviceMemoryBase.hpp
    include/DeviceObjectBase.hpp
    include/DeviceObjectArchive.hpp
    include/EngineFactoryBase.hpp
    include/EngineMemory.h
    include/FenceBase.hpp
    include/FramebufferBase.hpp
    include/IndexWrapper.hpp
    include/PipelineStateBase.hpp
    include/PipelineResourceSignatureBase.hpp
    include/PipelineStateCacheBase.hpp
    include/PrivateConstants.h
    include/PSOSerializer.hpp
    include/QueryBase.hpp
    include/RenderDeviceBase.hpp
    include/RenderPassBase.hpp
    include/ResourceMappingImpl.hpp
    include/SamplerBase.hpp
    include/ShaderBase.hpp
    include/ShaderResourceBindingBase.hpp
    include/ShaderResourceCacheCommon.hpp
    include/ShaderResourceVariableBase.hpp
    include/ShaderBindingTableBase.hpp
    include/SwapChainBase.hpp
    include/TextureBase.hpp
    include/TextureViewBase.hpp
    include/TopLevelASBase.hpp
)

set(INTERFACE 
    interface/APIInfo.h
    interface/BlendState.h
    interface/Buffer.h
    interface/BufferView.h
    interface/BottomLevelAS.h
    interface/CommandList.h
    interface/Constants.h
    interface/CommandQueue.h
    interface/Dearchiver.h
    interface/DepthStencilState.h
    interface/DeviceContext.h
    interface/DeviceMemory.h
    interface/DeviceObject.h
    interface/EngineFactory.h
    interface/Fence.h
    interface/Framebuffer.h
    interface/GraphicsTypes.h
    interface/GraphicsTypesX.hpp
    interface/InputLayout.h
    interface/PipelineState.h
    interface/PipelineResourceSignature.h
    interface/PipelineStateCache.h
    interface/Query.h
    interface/RasterizerState.h
    interface/RenderDevice.h
    interface/RenderPass.h
    interface/ResourceMapping.h
    interface/Sampler.h
    interface/Shader.h
    interface/ShaderResourceBinding.h
    interface/ShaderResourceVariable.h
    interface/ShaderBindingTable.h
    interface/SwapChain.h
    interface/Texture.h
    interface/TextureView.h
    interface/TopLevelAS.h
)

if(PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
    list(APPEND INTERFACE interface/LoadEngineDll.h)
endif()

set(SOURCE
    src/APIInfo.cpp
    src/BottomLevelASBase.cpp
    src/BufferBase.cpp
    src/DearchiverBase.cpp
    src/DefaultShaderSourceStreamFactory.cpp
    src/DeviceContextBase.cpp
    src/DeviceMemoryBase.cpp
    src/DeviceObjectArchive.cpp
    src/EngineMemory.cpp
    src/EngineFactoryBase.cpp
    src/FramebufferBase.cpp
    src/GraphicsTypesX.cpp
    src/PipelineResourceSignatureBase.cpp
    src/PipelineStateBase.cpp
    src/PipelineStateCacheBase.cpp
    src/PSOSerializer.cpp
    src/RenderDeviceBase.cpp
    src/ResourceMappingBase.cpp
    src/RenderPassBase.cpp
    src/ShaderBindingTableBase.cpp
    src/SamplerBase.cpp
    src/ShaderBase.cpp
    src/TextureBase.cpp
    src/TopLevelASBase.cpp
)

add_library(Diligent-GraphicsEngine STATIC ${SOURCE} ${INTERFACE} ${INCLUDE})
set_common_target_properties(Diligent-GraphicsEngine)

target_include_directories(Diligent-GraphicsEngine 
PUBLIC
    include
    interface
)

target_link_libraries(Diligent-GraphicsEngine 
PRIVATE
    Diligent-BuildSettings
PUBLIC
    Diligent-PlatformInterface
    Diligent-Common
    Diligent-GraphicsAccessories
)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngine PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngine)
endif()

find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=8 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE DILIGENT_CORE_COMMIT_HASH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(DILIGENT_CORE_COMMIT_HASH "${DILIGENT_CORE_COMMIT_HASH}" CACHE INTERNAL "" FORCE)
    message(STATUS "DiligentCore commit hash: " ${DILIGENT_CORE_COMMIT_HASH})
    target_compile_definitions(Diligent-GraphicsEngine PRIVATE DILIGENT_CORE_COMMIT_HASH="${DILIGENT_CORE_COMMIT_HASH}")
endif()
