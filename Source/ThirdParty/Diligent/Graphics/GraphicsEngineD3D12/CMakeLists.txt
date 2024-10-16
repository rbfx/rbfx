cmake_minimum_required (VERSION 3.11)

project(Diligent-GraphicsEngineD3D12 CXX)

set(INCLUDE
    include/BottomLevelASD3D12Impl.hpp
    include/BufferD3D12Impl.hpp
    include/BufferViewD3D12Impl.hpp
    include/CommandContext.hpp
    include/CommandListD3D12Impl.hpp
    include/CommandListManager.hpp
    include/CommandQueueD3D12Impl.hpp
    include/D3D12DynamicHeap.hpp
    include/D3D12TileMappingHelper.hpp
    include/D3D12ResourceBase.hpp
    include/D3D12TypeConversions.hpp
    include/D3D12TypeDefinitions.h
    include/D3D12Utils.h
    include/d3dx12_win.h
    include/DescriptorHeap.hpp
    include/DeviceContextD3D12Impl.hpp
    include/DeviceMemoryD3D12Impl.hpp
    include/DeviceObjectArchiveD3D12.hpp
    include/EngineD3D12ImplTraits.hpp
    include/FenceD3D12Impl.hpp
    include/FramebufferD3D12Impl.hpp
    include/GenerateMips.hpp
    include/pch.h
    include/PipelineResourceAttribsD3D12.hpp
    include/PipelineResourceSignatureD3D12Impl.hpp
    include/PipelineStateCacheD3D12Impl.hpp
    include/PipelineStateD3D12Impl.hpp
    include/QueryD3D12Impl.hpp
    include/QueryManagerD3D12.hpp
    include/RenderDeviceD3D12Impl.hpp
    include/RenderPassD3D12Impl.hpp
    include/RootParamsManager.hpp
    include/RootSignature.hpp
    include/SamplerD3D12Impl.hpp
    include/DearchiverD3D12Impl.hpp
    include/ShaderBindingTableD3D12Impl.hpp
    include/ShaderD3D12Impl.hpp
    include/ShaderResourceBindingD3D12Impl.hpp
    include/ShaderResourceCacheD3D12.hpp
    include/ShaderResourcesD3D12.hpp
    include/ShaderVariableManagerD3D12.hpp
    include/SwapChainD3D12Impl.hpp
    include/TextureD3D12Impl.hpp
    include/TextureViewD3D12Impl.hpp
    include/TopLevelASD3D12Impl.hpp
)

set(INTERFACE
    interface/BottomLevelASD3D12.h
    interface/BufferD3D12.h
    interface/BufferViewD3D12.h
    interface/CommandQueueD3D12.h
    interface/DeviceContextD3D12.h
    interface/DeviceMemoryD3D12.h
    interface/EngineFactoryD3D12.h
    interface/FenceD3D12.h
    interface/PipelineStateCacheD3D12.h
    interface/PipelineStateD3D12.h
    interface/QueryD3D12.h
    interface/RenderDeviceD3D12.h
    interface/SamplerD3D12.h
    interface/ShaderBindingTableD3D12.h
    interface/ShaderD3D12.h
    interface/ShaderResourceBindingD3D12.h
    interface/SwapChainD3D12.h
    interface/TextureD3D12.h
    interface/TextureViewD3D12.h
    interface/TopLevelASD3D12.h
)

set(SRC
    src/BottomLevelASD3D12Impl.cpp
    src/BufferD3D12Impl.cpp
    src/BufferViewD3D12Impl.cpp
    src/CommandContext.cpp
    src/CommandListManager.cpp
    src/CommandQueueD3D12Impl.cpp
    src/D3D12DynamicHeap.cpp
    src/D3D12TypeConversions.cpp
    src/D3D12Utils.cpp
    src/DescriptorHeap.cpp
    src/DeviceContextD3D12Impl.cpp
    src/DeviceMemoryD3D12Impl.cpp
    src/DeviceObjectArchiveD3D12.cpp
    src/EngineFactoryD3D12.cpp
    src/FenceD3D12Impl.cpp
    src/FramebufferD3D12Impl.cpp
    src/GenerateMips.cpp
    src/PipelineResourceSignatureD3D12Impl.cpp
    src/PipelineStateCacheD3D12Impl.cpp
    src/PipelineStateD3D12Impl.cpp
    src/QueryD3D12Impl.cpp
    src/QueryManagerD3D12.cpp
    src/RenderDeviceD3D12Impl.cpp
    src/RenderPassD3D12Impl.cpp
    src/RootParamsManager.cpp
    src/RootSignature.cpp
    src/SamplerD3D12Impl.cpp
    src/DearchiverD3D12Impl.cpp
    src/ShaderBindingTableD3D12Impl.cpp
    src/ShaderD3D12Impl.cpp
    src/ShaderResourceBindingD3D12Impl.cpp
    src/ShaderResourceCacheD3D12.cpp
    src/ShaderResourcesD3D12.cpp
    src/ShaderVariableManagerD3D12.cpp
    src/SwapChainD3D12Impl.cpp
    src/TextureD3D12Impl.cpp
    src/TextureViewD3D12Impl.cpp
    src/TopLevelASD3D12Impl.cpp
)

set(DLL_SRC
    src/DLLMain.cpp
    src/GraphicsEngineD3D12.def
)

if(PLATFORM_WIN32)
    list(APPEND INCLUDE include/D3D12Loader.hpp)
    list(APPEND SRC src/D3D12Loader.cpp)
    set(USE_D3D12_LOADER 1)
else()
    set(USE_D3D12_LOADER 0)
endif()

set(SHADERS
    shaders/GenerateMips/GenerateMipsGammaCS.hlsl
    shaders/GenerateMips/GenerateMipsGammaOddCS.hlsl
    shaders/GenerateMips/GenerateMipsGammaOddXCS.hlsl
    shaders/GenerateMips/GenerateMipsGammaOddYCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearOddCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearOddXCS.hlsl
    shaders/GenerateMips/GenerateMipsLinearOddYCS.hlsl
)
set_source_files_properties(${SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")

# Set up custom commands to compile shaders
set(COMPILED_SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/CompiledShaders/GenerateMips)
file(MAKE_DIRECTORY "${COMPILED_SHADERS_DIR}")

foreach(SRC_SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SRC_SHADER} NAME_WE)
    set(COMPILED_SHADER ${COMPILED_SHADERS_DIR}/${SHADER_NAME}.h)
    list(APPEND COMPILED_SHADERS ${COMPILED_SHADER})

    add_custom_command(OUTPUT ${COMPILED_SHADER} # We must use full path here!
                       COMMAND fxc /T cs_5_0 /E main /Vn g_p${SHADER_NAME} /Fh "${COMPILED_SHADER}" "${SRC_SHADER}"
                       WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                       MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/shaders/GenerateMips/GenerateMipsCS.hlsli"
                       COMMENT "Compiling ${SRC_SHADER}"
                       VERBATIM
    )
endforeach(SRC_SHADER)

# NB: we must use the full path, otherwise the build system will not be able to properly detect
#     changes and shader compilation custom command will run every time
set_source_files_properties(${COMPILED_SHADERS} PROPERTIES GENERATED TRUE)

add_library(Diligent-GraphicsEngineD3D12Interface INTERFACE)
target_link_libraries     (Diligent-GraphicsEngineD3D12Interface INTERFACE Diligent-GraphicsEngineD3DBaseInterface)
target_include_directories(Diligent-GraphicsEngineD3D12Interface INTERFACE interface)


add_library(Diligent-GraphicsEngineD3D12-static STATIC
    ${SRC} ${INTERFACE} ${INCLUDE} ${SHADERS}
    readme.md
    shaders/GenerateMips/GenerateMipsCS.hlsli
    # A target created in the same directory (CMakeLists.txt file) that specifies any output of the
    # custom command as a source file is given a rule to generate the file using the command at build time.
    ${COMPILED_SHADERS}
)

add_library(Diligent-GraphicsEngineD3D12-shared SHARED
    ${DLL_SRC}
    readme.md
)

target_include_directories(Diligent-GraphicsEngineD3D12-static
PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/CompiledShaders
    include
)

target_link_libraries(Diligent-GraphicsEngineD3D12-static
PRIVATE
    Diligent-BuildSettings
    Diligent-Common
    Diligent-GraphicsEngineD3DBase
    Diligent-GraphicsEngineNextGenBase
    Diligent-TargetPlatform
    Diligent-ShaderTools
    dxgi.lib
    d3dcompiler.lib
PUBLIC
    Diligent-GraphicsEngineD3D12Interface
)

target_compile_definitions(Diligent-GraphicsEngineD3D12-static PRIVATE USE_D3D12_LOADER=${USE_D3D12_LOADER})
if(NOT ${USE_D3D12_LOADER})
    # Link with d3d12.lib if we don't use d3d12 loader
    target_link_libraries(Diligent-GraphicsEngineD3D12-static PRIVATE d3d12.lib)
endif()

target_link_libraries(Diligent-GraphicsEngineD3D12-shared
PRIVATE
    Diligent-BuildSettings
    Diligent-GraphicsEngineD3D12-static
PUBLIC
    Diligent-GraphicsEngineD3D12Interface
)
target_compile_definitions(Diligent-GraphicsEngineD3D12-shared PUBLIC ENGINE_DLL=1)

if("${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}" VERSION_GREATER_EQUAL "10.0.19041.0")
    set(D3D12_H_HAS_MESH_SHADER ON CACHE INTERNAL "D3D12 headers support mesh shaders" FORCE)
    target_compile_definitions(Diligent-GraphicsEngineD3D12-static PRIVATE D3D12_H_HAS_MESH_SHADER=1)
endif()

# Set output name to GraphicsEngineD3D12_{32|64}{r|d}
set_dll_output_name(Diligent-GraphicsEngineD3D12-shared GraphicsEngineD3D12)

set_common_target_properties(Diligent-GraphicsEngineD3D12-shared)
set_common_target_properties(Diligent-GraphicsEngineD3D12-static)

source_group("src" FILES ${SRC})
source_group("dll" FILES ${DLL_SRC})

source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("shaders" FILES
    ${SHADERS}
    shaders/GenerateMips/GenerateMipsCS.hlsli
)
source_group("generated" FILES ${COMPILED_SHADERS})


option(DILIGENT_LOAD_PIX_EVENT_RUNTIME OFF "Download WinPixEventRuntime to enable Direct3D12 debug utils")
if(${DILIGENT_LOAD_PIX_EVENT_RUNTIME})
    include(FetchContent)
    set(DILIGENT_PIX_EVENT_RUNTIME_PATH "${CMAKE_BINARY_DIR}/WinPixEventRuntime")
    FetchContent_Declare(
        WinPixEventRuntime
        URL                 "https://www.nuget.org/api/v2/package/WinPixEventRuntime"
        DOWNLOAD_DIR        ${DILIGENT_PIX_EVENT_RUNTIME_PATH}
        LOG_DOWNLOAD        1
        SOURCE_DIR			${DILIGENT_PIX_EVENT_RUNTIME_PATH}
        CONFIGURE_COMMAND	${CMAKE_COMMAND} -E tar -xf "${CMAKE_BINARY_DIR}/WinPixEventRuntime/winpixeventruntime.*.nupkg"
        LOG_CONFIGURE 		1
    )
    FetchContent_GetProperties(WinPixEventRuntime)
    if(NOT WinPixEventRuntime_POPULATED)
        FetchContent_Populate(WinPixEventRuntime)
    endif()
    if(EXISTS "${DILIGENT_PIX_EVENT_RUNTIME_PATH}/include/WinPixEventRuntime/pix3.h")
        if(PLATFORM_WIN32 AND ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
            set(DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH ${DILIGENT_PIX_EVENT_RUNTIME_PATH}/bin/x64/WinPixEventRuntime.dll)
            set(DILIGENT_PIX_EVENT_RUNTIME_LIB_PATH ${DILIGENT_PIX_EVENT_RUNTIME_PATH}/bin/x64/WinPixEventRuntime.lib)
        elseif(PLATFORM_UNIVERSAL_WINDOWS AND ${CMAKE_SIZEOF_VOID_P} EQUAL 8)
            set(DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH ${DILIGENT_PIX_EVENT_RUNTIME_PATH}/bin/x64/WinPixEventRuntime_UAP.dll)
            set(DILIGENT_PIX_EVENT_RUNTIME_LIB_PATH ${DILIGENT_PIX_EVENT_RUNTIME_PATH}/bin/x64/WinPixEventRuntime_UAP.lib)
        else()
            message(STATUS "WinPixEventRuntime is not supported in this configuration")
            set(DILIGENT_PIX_EVENT_RUNTIME_PATH_VALID OFF)
        endif()

        if (EXISTS "${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH}" AND EXISTS "${DILIGENT_PIX_EVENT_RUNTIME_LIB_PATH}")
            message(STATUS "Found WinPixEventRuntime")
            target_compile_definitions(Diligent-GraphicsEngineD3D12-static PRIVATE DILIGENT_USE_PIX)
            target_include_directories(Diligent-GraphicsEngineD3D12-static PRIVATE "${DILIGENT_PIX_EVENT_RUNTIME_PATH}")
            target_link_libraries(Diligent-GraphicsEngineD3D12-static PRIVATE "${DILIGENT_PIX_EVENT_RUNTIME_LIB_PATH}")
            set(DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH ${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH} CACHE PATH "Win Pix Event Runtime DLL path")
        else()
            if(NOT EXISTS "${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH}")
                message(STATUS "'${DILIGENT_PIX_EVENT_RUNTIME_DLL_PATH}' is not valid. Pix events will be disabled")
            endif()
            if(NOT EXISTS "${DILIGENT_PIX_EVENT_RUNTIME_LIB_PATH}")
                message(STATUS "'${DILIGENT_PIX_EVENT_RUNTIME_LIB_PATH}' is not valid. Pix events will be disabled")
            endif()
        endif()
    endif()
endif()

set_target_properties(Diligent-GraphicsEngineD3D12-static PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GraphicsEngineD3D12-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineD3D12-shared)
    install_core_lib(Diligent-GraphicsEngineD3D12-static)
endif()
