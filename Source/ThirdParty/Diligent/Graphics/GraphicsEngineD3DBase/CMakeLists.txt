cmake_minimum_required (VERSION 3.10)

project(Diligent-GraphicsEngineD3DBase CXX)

set(DILIGENT_NVAPI_PATH "" CACHE PATH "Path to NVAPI to enable D3D11/D3D12 extensions")

set(INCLUDE
    include/D3DCommonTypeConversions.hpp
    include/D3DErrors.hpp
    include/D3DTileMappingHelper.hpp
    include/D3DShaderResourceLoader.hpp
    include/D3DShaderResourceValidation.hpp
    include/D3DTypeConversionImpl.hpp
    include/D3DViewDescConversionImpl.hpp
    include/DXGITypeConversions.hpp
    include/EngineFactoryD3DBase.hpp
    include/RenderDeviceD3DBase.hpp
    include/ShaderD3DBase.hpp
    include/ShaderResources.hpp
    include/ShaderVariableD3D.hpp
    include/SwapChainD3DBase.hpp
    include/NVApiLoader.hpp
)

set(INTERFACE
    interface/ShaderD3D.h
)

set(SOURCE
    src/D3DCommonTypeConversions.cpp
    src/D3DShaderResourceValidation.cpp
    src/DXGITypeConversions.cpp
    src/ShaderD3DBase.cpp
    src/ShaderResources.cpp
)

add_library(Diligent-GraphicsEngineD3DBase STATIC
    ${SOURCE} ${INCLUDE} ${INTERFACE}
)

add_library(Diligent-GraphicsEngineD3DBaseInterface INTERFACE)
target_link_libraries     (Diligent-GraphicsEngineD3DBaseInterface INTERFACE Diligent-GraphicsEngineInterface)
target_include_directories(Diligent-GraphicsEngineD3DBaseInterface INTERFACE interface)

target_include_directories(Diligent-GraphicsEngineD3DBase
PUBLIC
    include
)

target_link_libraries(Diligent-GraphicsEngineD3DBase
PRIVATE
    Diligent-BuildSettings
    Diligent-ShaderTools
PUBLIC
    Diligent-GraphicsEngine
    Diligent-GraphicsEngineD3DBaseInterface
)

set_common_target_properties(Diligent-GraphicsEngineD3DBase)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngineD3DBase PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineD3DBase)
endif()

# Set paths to D3Dcompiler_47.dll, dxcompiler.dll, and dxil.dll
if(MSVC)
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(ARCH_SUFFIX "x64")
    else()
        set(ARCH_SUFFIX "x86")
    endif()

    # Note that CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION is stated to be defined when targeting Windows 10
    # and above, however it is also defined when targeting 8.1 and Visual Studio 2019 (but not VS2017)
    if("${WINDOWS_SDK_VERSION}" VERSION_GREATER_EQUAL "10.0")
        if (DEFINED CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
            # Note that VS_WINDOWS_SDK_BIN_DIR as well as all derived paths can only be used in Visual Studio
            # commands and are not valid paths during CMake configuration
            set(VS_WINDOWS_SDK_BIN_DIR "$(WindowsSdkDir)\\bin\\${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\\${ARCH_SUFFIX}")

            set(VS_D3D_COMPILER_PATH "\"${VS_WINDOWS_SDK_BIN_DIR}\\D3Dcompiler_47.dll\"" CACHE INTERNAL "D3Dcompiler_47.dll path")

            # DXC is only present in Windows SDK starting with version 10.0.17763.0
            if(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} VERSION_GREATER_EQUAL "10.0.17763.0")
                set(VS_DXC_COMPILER_PATH "\"${VS_WINDOWS_SDK_BIN_DIR}\\dxcompiler.dll\"" CACHE INTERNAL "dxcompiler.dll path")
                set(VS_DXIL_SIGNER_PATH  "\"${VS_WINDOWS_SDK_BIN_DIR}\\dxil.dll\"" CACHE INTERNAL "dxil.dll path")
            endif()
        endif()
    elseif("${WINDOWS_SDK_VERSION}" VERSION_EQUAL "8.1")
        # D3Dcompiler_47.dll from Win8.1 SDK is ancient (from 2013) and fails to
        # compile a number of test shaders. Use the compiler from Visual Studio
        # executable path instead
        set(VS_D3D_COMPILER_PATH "\"$(VC_ExecutablePath_x64_${ARCH_SUFFIX})\\D3Dcompiler_47.dll\"" CACHE INTERNAL "D3Dcompiler_47.dll path")
    endif()
endif()


if(EXISTS "${DILIGENT_NVAPI_PATH}/nvapi.h")
    message(STATUS "Using NVApi from ${DILIGENT_NVAPI_PATH}")
    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(DILIGENT_NVAPI_LIB_PATH "${DILIGENT_NVAPI_PATH}/amd64/nvapi64.lib" CACHE INTERNAL "NVAPI lib path")
    else()
        set(DILIGENT_NVAPI_LIB_PATH "${DILIGENT_NVAPI_PATH}/x86/nvapi.lib" CACHE INTERNAL "NVAPI lib path")
    endif()
    target_include_directories(Diligent-GraphicsEngineD3DBase PUBLIC ${DILIGENT_NVAPI_PATH})
    target_link_libraries(Diligent-GraphicsEngineD3DBase PRIVATE ${DILIGENT_NVAPI_LIB_PATH})
    target_compile_definitions(Diligent-GraphicsEngineD3DBase PUBLIC DILIGENT_ENABLE_D3D_NVAPI)
endif()
