cmake_minimum_required (VERSION 3.10)

project(Diligent-GraphicsEngineVk CXX)

set(INCLUDE
    include/BufferVkImpl.hpp
    include/BufferViewVkImpl.hpp
    include/BottomLevelASVkImpl.hpp
    include/CommandListVkImpl.hpp
    include/CommandPoolManager.hpp
    include/CommandQueueVkImpl.hpp
    include/DescriptorPoolManager.hpp
    include/DeviceContextVkImpl.hpp
    include/DeviceMemoryVkImpl.hpp
    include/DeviceObjectArchiveVk.hpp
    include/EngineVkImplTraits.hpp
    include/FenceVkImpl.hpp
    include/FramebufferVkImpl.hpp
    include/FramebufferCache.hpp
    include/GenerateMipsVkHelper.hpp
    include/ManagedVulkanObject.hpp
    include/pch.h
    include/PipelineLayoutVk.hpp
    include/PipelineStateVkImpl.hpp
    include/PipelineResourceSignatureVkImpl.hpp
    include/PipelineResourceAttribsVk.hpp
    include/PipelineStateCacheVkImpl.hpp
    include/QueryManagerVk.hpp
    include/QueryVkImpl.hpp
    include/RenderDeviceVkImpl.hpp
    include/RenderPassVkImpl.hpp
    include/RenderPassCache.hpp
    include/SamplerVkImpl.hpp
    include/DearchiverVkImpl.hpp
    include/ShaderVkImpl.hpp
    include/ShaderResourceBindingVkImpl.hpp
    include/ShaderResourceCacheVk.hpp
    include/ShaderVariableManagerVk.hpp
    include/ShaderBindingTableVkImpl.hpp
    include/SwapChainVkImpl.hpp
    include/TextureVkImpl.hpp
    include/TextureViewVkImpl.hpp
    include/TopLevelASVkImpl.hpp
    include/VulkanDynamicHeap.hpp
    include/VulkanErrors.hpp
    include/VulkanTypeConversions.hpp
    include/VulkanUploadHeap.hpp
)

set(VULKAN_UTILS_INCLUDE
    include/VulkanUtilities/VulkanCommandBuffer.hpp
    include/VulkanUtilities/VulkanCommandBufferPool.hpp
    include/VulkanUtilities/VulkanDebug.hpp
    include/VulkanUtilities/VulkanInstance.hpp
    include/VulkanUtilities/VulkanLogicalDevice.hpp
    include/VulkanUtilities/VulkanMemoryManager.hpp
    include/VulkanUtilities/VulkanObjectWrappers.hpp
    include/VulkanUtilities/VulkanPhysicalDevice.hpp
    include/VulkanUtilities/VulkanSyncObjectManager.hpp
    include/VulkanUtilities/VulkanHeaders.h
)


set(INTERFACE
    interface/BufferVk.h
    interface/BufferViewVk.h
    interface/BottomLevelASVk.h
    interface/CommandQueueVk.h
    interface/DeviceContextVk.h
    interface/DeviceMemoryVk.h
    interface/EngineFactoryVk.h
    interface/FenceVk.h
    interface/FramebufferVk.h
    interface/PipelineStateVk.h
    interface/PipelineStateCacheVk.h
    interface/QueryVk.h
    interface/RenderDeviceVk.h
    interface/RenderPassVk.h
    interface/SamplerVk.h
    interface/ShaderVk.h
    interface/ShaderResourceBindingVk.h
    interface/ShaderBindingTableVk.h
    interface/SwapChainVk.h
    interface/TextureVk.h
    interface/TextureViewVk.h
    interface/TopLevelASVk.h
)


set(SRC
    src/BufferVkImpl.cpp
    src/BufferViewVkImpl.cpp
    src/BottomLevelASVkImpl.cpp
    src/CommandPoolManager.cpp
    src/CommandQueueVkImpl.cpp
    src/DescriptorPoolManager.cpp
    src/DeviceContextVkImpl.cpp
    src/DeviceMemoryVkImpl.cpp
    src/DeviceObjectArchiveVk.cpp
    src/EngineFactoryVk.cpp
    src/FenceVkImpl.cpp
    src/FramebufferVkImpl.cpp
    src/FramebufferCache.cpp
    src/GenerateMipsVkHelper.cpp
    src/PipelineLayoutVk.cpp
    src/PipelineStateVkImpl.cpp
    src/PipelineResourceSignatureVkImpl.cpp
    src/PipelineStateCacheVkImpl.cpp
    src/QueryManagerVk.cpp
    src/QueryVkImpl.cpp
    src/RenderDeviceVkImpl.cpp
    src/RenderPassVkImpl.cpp
    src/RenderPassCache.cpp
    src/SamplerVkImpl.cpp
    src/DearchiverVkImpl.cpp
    src/ShaderVkImpl.cpp
    src/ShaderResourceBindingVkImpl.cpp
    src/ShaderResourceCacheVk.cpp
    src/ShaderVariableManagerVk.cpp
    src/ShaderBindingTableVkImpl.cpp
    src/SwapChainVkImpl.cpp
    src/TextureVkImpl.cpp
    src/TextureViewVkImpl.cpp
    src/TopLevelASVkImpl.cpp
    src/VulkanDynamicHeap.cpp
    src/VulkanTypeConversions.cpp
    src/VulkanUploadHeap.cpp
)

set(VULKAN_UTILS_SRC
    src/VulkanUtilities/VulkanCommandBuffer.cpp
    src/VulkanUtilities/VulkanCommandBufferPool.cpp
    src/VulkanUtilities/VulkanDebug.cpp
    src/VulkanUtilities/VulkanInstance.cpp
    src/VulkanUtilities/VulkanLogicalDevice.cpp
    src/VulkanUtilities/VulkanMemoryManager.cpp
    src/VulkanUtilities/VulkanPhysicalDevice.cpp
    src/VulkanUtilities/VulkanSyncObjectManager.cpp
)

set(DLL_SOURCE
    src/DLLMain.cpp
    src/GraphicsEngineVk.def
)

add_library(Diligent-GraphicsEngineVkInterface INTERFACE)
target_link_libraries     (Diligent-GraphicsEngineVkInterface INTERFACE Diligent-GraphicsEngineInterface)
target_include_directories(Diligent-GraphicsEngineVkInterface INTERFACE interface)


add_library(Diligent-GraphicsEngineVk-static STATIC
    ${SRC} ${VULKAN_UTILS_SRC} ${INTERFACE} ${INCLUDE} ${VULKAN_UTILS_INCLUDE}
    readme.md
)

add_library(Diligent-GraphicsEngineVk-shared SHARED
    readme.md
)

if(MSVC)
    target_sources(Diligent-GraphicsEngineVk-shared PRIVATE ${DLL_SOURCE})
endif()

target_include_directories(Diligent-GraphicsEngineVk-static
PRIVATE
    include
)

set(PRIVATE_DEPENDENCIES
    Diligent-BuildSettings
    Diligent-Common
    Diligent-TargetPlatform
    Diligent-GraphicsEngineNextGenBase
    Diligent-ShaderTools
    Vulkan::Headers
)

if (${DILIGENT_NO_HLSL})
    message("HLSL support is disabled. Vulkan backend will not be able to consume SPIRV bytecode generated from HLSL.")
else()
    list(APPEND PRIVATE_DEPENDENCIES SPIRV-Tools-opt)
endif()

# Use DirectX shader compiler for SPIRV.
# This is another implementation of DXC that can compile only to SPIRV.
# DXC for D3D12 can compile only to DXIL.
if(PLATFORM_WIN32)
    # find Vulkan SDK
    find_package(Vulkan)
    if(Vulkan_FOUND)
        get_filename_component(VulkanSDKPath ${Vulkan_LIBRARY} DIRECTORY)
        set(VulkanSDKPath "${VulkanSDKPath}/..")
        if(NOT EXISTS "${VulkanSDKPath}")
            message(STATUS "can't get Vulkan SDK path")
        endif()
        if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
            set(DXC_SPIRV_PATH "${VulkanSDKPath}/Bin/dxcompiler.dll")
        else()
            set(DXC_SPIRV_PATH "${VulkanSDKPath}/Bin32/dxcompiler.dll")
        endif()
    endif()
    # Try to find Vulkan SDK from environment variable
    if(NOT Vulkan_FOUND)
        if(EXISTS "$ENV{VULKAN_SDK}")
            if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
                set(DXC_SPIRV_PATH "$ENV{VULKAN_SDK}/Bin/dxcompiler.dll")
            else()
                set(DXC_SPIRV_PATH "$ENV{VULKAN_SDK}/Bin32/dxcompiler.dll")
            endif()
        endif()
    endif()
elseif(PLATFORM_MACOS)
    if(NOT VULKAN_SDK AND DEFINED ENV{VULKAN_SDK})
        set(VULKAN_SDK $ENV{VULKAN_SDK})
    endif()

    if(VULKAN_SDK)
        if(EXISTS "${VULKAN_SDK}/lib/libvulkan.dylib")
            set(VULKAN_LIB_PATH ${VULKAN_SDK}/lib CACHE PATH "Vulkan library path" FORCE)
        elseif(EXISTS "${VULKAN_SDK}/macOS/lib/libvulkan.dylib")
            set(VULKAN_LIB_PATH ${VULKAN_SDK}/macOS/lib CACHE PATH "Vulkan library path" FORCE)
        endif()

        if(NOT VULKAN_LIB_PATH)
            message(WARNING "Unable to find vulkan library at: " ${VULKAN_SDK})
        endif()
    endif()

    if (VULKAN_LIB_PATH)
        message("Using Vulkan library path: " ${VULKAN_LIB_PATH})
    else()
        message(WARNING "Vulkan library is not found. Executables may fail to start because the library may not be in rpath. Define VULKAN_SDK environment or CMake variable to specify the SDK path.")
    endif()
endif()

if(PLATFORM_LINUX)
    # VulkanSDK installs dxc to this path
    set(DXC_SPIRV_PATH "/usr/lib/dxc/libdxcompiler.so")
endif()

if(EXISTS ${DXC_SPIRV_PATH})
    set(DILIGENT_DXCOMPILER_FOR_SPIRV_PATH "${DXC_SPIRV_PATH}" CACHE INTERNAL "" FORCE)
    message(STATUS "Found DXCompiler for Vulkan: " ${DILIGENT_DXCOMPILER_FOR_SPIRV_PATH})
else()
    set(DILIGENT_DXCOMPILER_FOR_SPIRV_PATH "" CACHE INTERNAL "" FORCE)
endif()

if(PLATFORM_WIN32)
    list(APPEND PRIVATE_DEPENDENCIES volk::volk_headers)
elseif(PLATFORM_MACOS)
    list(APPEND PRIVATE_DEPENDENCIES volk::volk_headers)
elseif(PLATFORM_IOS OR PLATFORM_TVOS)
    find_library(CORE_GRAPHICS CoreGraphics)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZ_CORE QuartzCore)
    find_library(UI_KIT UIKit)
    find_library(IO_SURFACE IOSurface)
    # Note that volk may be defined by external CMake, but we don't use it on iOS/tvOS
    list(APPEND PRIVATE_DEPENDENCIES ${MOLTENVK_LIBRARY} ${CORE_GRAPHICS} ${METAL_LIBRARY} ${QUARTZ_CORE} ${UI_KIT} ${IO_SURFACE})
elseif(PLATFORM_LINUX)
    list(APPEND PRIVATE_DEPENDENCIES volk::volk_headers)
elseif(PLATFORM_ANDROID)
    list(APPEND PRIVATE_DEPENDENCIES volk::volk_headers)
else()
    find_library(Vulkan_LIBRARY NAMES vulkan)
    list(APPEND PRIVATE_DEPENDENCIES ${Vulkan_LIBRARY})
endif()

set(PUBLIC_DEPENDENCIES
    Diligent-GraphicsEngineVkInterface
)

target_link_libraries(Diligent-GraphicsEngineVk-static
PRIVATE
    ${PRIVATE_DEPENDENCIES}
PUBLIC
    ${PUBLIC_DEPENDENCIES}
)
target_link_libraries(Diligent-GraphicsEngineVk-shared
PRIVATE
    Diligent-BuildSettings
    ${WHOLE_ARCHIVE_FLAG} Diligent-GraphicsEngineVk-static ${NO_WHOLE_ARCHIVE_FLAG}
PUBLIC
    ${PUBLIC_DEPENDENCIES}
)

if (MINGW_BUILD)
    # Restrict export to GetEngineFactoryVk
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/export.map
        "{ global: *GetEngineFactoryVk*; local: *; };"
    )
    # set_target_properties does not append link flags, but overwrites them
    target_link_options(Diligent-GraphicsEngineVk-shared PRIVATE LINKER:--version-script=export.map)
endif()

if(PLATFORM_WIN32)
    set(PRIVATE_COMPILE_DEFINITIONS VK_USE_PLATFORM_WIN32_KHR=1 NOMINMAX DILIGENT_USE_VOLK=1)
elseif(PLATFORM_LINUX)
    set(PRIVATE_COMPILE_DEFINITIONS VK_USE_PLATFORM_XCB_KHR=1 VK_USE_PLATFORM_XLIB_KHR=1 DILIGENT_USE_VOLK=1)
elseif(PLATFORM_MACOS)
    set(PRIVATE_COMPILE_DEFINITIONS VK_USE_PLATFORM_METAL_EXT=1 DILIGENT_USE_VOLK=1)
elseif(PLATFORM_IOS OR PLATFORM_TVOS)
    set(PRIVATE_COMPILE_DEFINITIONS VK_USE_PLATFORM_METAL_EXT=1)
elseif(PLATFORM_ANDROID)
    set(PRIVATE_COMPILE_DEFINITIONS VK_USE_PLATFORM_ANDROID_KHR=1 DILIGENT_USE_VOLK=1)
else()
    message(FATAL_ERROR "Unknown platform")
endif()

if (${DILIGENT_NO_GLSLANG})
    message("GLSLang is not being built. Vulkan backend will only be able to consume SPIRV byte code.")
endif()

target_compile_definitions(Diligent-GraphicsEngineVk-static
PRIVATE
    ${PRIVATE_COMPILE_DEFINITIONS}
    DILIGENT_NO_GLSLANG=$<BOOL:${DILIGENT_NO_GLSLANG}>
    DILIGENT_NO_HLSL=$<BOOL:${DILIGENT_NO_HLSL}>
)
target_compile_definitions(Diligent-GraphicsEngineVk-shared PRIVATE ${PRIVATE_COMPILE_DEFINITIONS} ENGINE_DLL=1)

if(PLATFORM_WIN32)

    # Do not add 'lib' prefix when building with MinGW
    set_target_properties(Diligent-GraphicsEngineVk-shared PROPERTIES PREFIX "")

    # Set output name to GraphicsEngineVk_{32|64}{r|d}
    set_dll_output_name(Diligent-GraphicsEngineVk-shared GraphicsEngineVk)

else()
    set_target_properties(Diligent-GraphicsEngineVk-shared PROPERTIES
        OUTPUT_NAME GraphicsEngineVk
    )
endif()

set_common_target_properties(Diligent-GraphicsEngineVk-shared)
set_common_target_properties(Diligent-GraphicsEngineVk-static)

source_group("src" FILES ${SRC})
source_group("src\\Vulkan Utilities" FILES ${VULKAN_UTILS_SRC})
source_group("dll" FILES ${DLL_SOURCE})

source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("include\\Vulkan Utilities" FILES ${VULKAN_UTILS_INCLUDE})

set_target_properties(Diligent-GraphicsEngineVk-static PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GraphicsEngineVk-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineVk-shared)
    install_core_lib(Diligent-GraphicsEngineVk-static)
endif()
