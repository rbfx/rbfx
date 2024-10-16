cmake_minimum_required (VERSION 3.6)

project(Diligent-GraphicsTools CXX)

set(INTERFACE
    interface/BufferSuballocator.h
    interface/BytecodeCache.h
    interface/CommonlyUsedStates.h
    interface/DynamicBuffer.hpp
    interface/DynamicTextureArray.hpp
    interface/DynamicTextureAtlas.h
    interface/DurationQueryHelper.hpp
    interface/GraphicsUtilities.h
    interface/MapHelper.hpp
    interface/OffScreenSwapChain.hpp
    interface/ResourceRegistry.hpp
    interface/ScopedDebugGroup.hpp
    interface/GPUCompletionAwaitQueue.hpp
    interface/ScopedQueryHelper.hpp
    interface/ScreenCapture.hpp
    interface/ShaderMacroHelper.hpp
    interface/StreamingBuffer.hpp
    interface/ShaderSourceFactoryUtils.h
    interface/ShaderSourceFactoryUtils.hpp
    interface/TextureUploader.hpp
    interface/TextureUploaderBase.hpp
    interface/XXH128Hasher.hpp
    interface/VertexPool.h
    interface/VertexPoolX.hpp
)

set(SOURCE
    src/BufferSuballocator.cpp
    src/BytecodeCache.cpp
    src/DurationQueryHelper.cpp
    src/DynamicBuffer.cpp
    src/DynamicTextureArray.cpp
    src/DynamicTextureAtlas.cpp
    src/GraphicsUtilities.cpp
    src/GraphicsUtilitiesD3D11.cpp
    src/GraphicsUtilitiesD3D12.cpp
    src/GraphicsUtilitiesGL.cpp
    src/GraphicsUtilitiesVk.cpp
    src/GraphicsUtilitiesWebGPU.cpp
    src/OffScreenSwapChain.cpp
    src/ScopedQueryHelper.cpp
    src/ScreenCapture.cpp
    src/ShaderSourceFactoryUtils.cpp
    src/TextureUploader.cpp
    src/XXH128Hasher.cpp
    src/VertexPool.cpp
)

set(INCLUDE include/ProxyPipelineState.hpp)

if(ARCHIVER_SUPPORTED)
    list(APPEND INTERFACE
        interface/RenderStateCache.h
        interface/RenderStateCache.hpp
    )
    list(APPEND INCLUDE
        include/AsyncPipelineState.hpp
        include/RenderStateCacheImpl.hpp
        include/ReloadableShader.hpp
        include/ReloadablePipelineState.hpp
    )
    list(APPEND SOURCE
        src/AsyncPipelineState.cpp
        src/RenderStateCacheImpl.cpp
        src/ReloadableShader.cpp
        src/ReloadablePipelineState.cpp
    )
    set(RENDER_STATE_CACHE_SUPPORTED TRUE CACHE INTERNAL "Render state cache is supported")
else()
    set(RENDER_STATE_CACHE_SUPPORTED FALSE CACHE INTERNAL "Render state cache is not supported")
endif()

set(DEPENDENCIES)

if(D3D11_SUPPORTED)
    list(APPEND SOURCE src/TextureUploaderD3D11.cpp)
    list(APPEND INTERFACE interface/TextureUploaderD3D11.hpp)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineD3D11Interface)
endif()

if(D3D12_SUPPORTED)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineD3D12Interface)
endif()

if(VULKAN_SUPPORTED)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineVkInterface Vulkan::Headers)
endif()

if(D3D12_SUPPORTED OR VULKAN_SUPPORTED)
    list(APPEND SOURCE src/TextureUploaderD3D12_Vk.cpp)
    list(APPEND INTERFACE interface/TextureUploaderD3D12_Vk.hpp)
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    list(APPEND SOURCE src/TextureUploaderGL.cpp)
    list(APPEND INTERFACE interface/TextureUploaderGL.hpp)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineOpenGLInterface)
endif()

if(WEBGPU_SUPPORTED)
    list(APPEND SOURCE src/TextureUploaderWebGPU.cpp)
    list(APPEND INTERFACE interface/TextureUploaderWebGPU.hpp)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineWebGPUInterface)
    if (NOT PLATFORM_EMSCRIPTEN)
        list(APPEND DEPENDENCIES dawn_proc)
    endif()
endif()

if(METAL_SUPPORTED)
    list(APPEND SOURCE src/GraphicsUtilitiesMtl.mm)
    list(APPEND DEPENDENCIES Diligent-GraphicsEngineMetalInterface)
endif()

add_library(Diligent-GraphicsTools STATIC ${SOURCE} ${INCLUDE} ${INTERFACE})

target_include_directories(Diligent-GraphicsTools
PUBLIC
    interface
PRIVATE
    ../GraphicsEngineD3DBase/include
    include
)

target_link_libraries(Diligent-GraphicsTools
PRIVATE
    Diligent-Common
    Diligent-BuildSettings
    Diligent-PlatformInterface
    Diligent-GraphicsAccessories
    Diligent-ShaderTools
    Diligent-GraphicsEngine
    xxHash::xxhash
    ${DEPENDENCIES}
PUBLIC
    Diligent-GraphicsEngineInterface
)

if(ARCHIVER_SUPPORTED)
    target_link_libraries(Diligent-GraphicsTools
    PRIVATE
        Diligent-Archiver-static
    )
endif()

if(RENDER_STATE_CACHE_SUPPORTED)
    target_compile_definitions(Diligent-GraphicsTools PUBLIC DILIGENT_RENDER_STATE_CACHE_SUPPORTED=1)
endif()

if(D3D11_SUPPORTED OR D3D12_SUPPORTED)
    target_link_libraries(Diligent-GraphicsTools
    PRIVATE
        Diligent-GraphicsEngineD3DBase
    )
endif()

set_common_target_properties(Diligent-GraphicsTools)

source_group("src" FILES ${SOURCE})
source_group("interface" FILES ${INTERFACE})
source_group("include" FILES ${INCLUDE})

set_target_properties(Diligent-GraphicsTools PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsTools)
endif()
