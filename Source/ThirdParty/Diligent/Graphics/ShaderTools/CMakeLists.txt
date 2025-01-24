cmake_minimum_required (VERSION 3.6)

project(Diligent-ShaderTools CXX)

set(INCLUDE
    include/ShaderToolsCommon.hpp
    include/GLSLParsingTools.hpp
    include/HLSLParsingTools.hpp
    include/HLSLTokenizer.hpp
    include/HLSLDefinitions.fxh
    include/HLSLKeywords.h
)

set(SOURCE
    src/ShaderToolsCommon.cpp
    src/GLSLParsingTools.cpp
    src/HLSLParsingTools.cpp
    src/HLSLTokenizer.cpp
)

set(DXC_SUPPORTED FALSE)
if((PLATFORM_WIN32 AND NOT MINGW_BUILD) OR PLATFORM_UNIVERSAL_WINDOWS OR PLATFORM_LINUX)
    set(DXC_SUPPORTED TRUE)
endif()

set(ENABLE_GLSL FALSE)
if(VULKAN_SUPPORTED OR GL_SUPPORTED OR GLES_SUPPORTED OR METAL_SUPPORTED OR WEBGPU_SUPPORTED)
    set(ENABLE_GLSL TRUE)
endif()

set(ENABLE_HLSL FALSE)
if(D3D11_SUPPORTED OR D3D12_SUPPORTED OR VULKAN_SUPPORTED OR METAL_SUPPORTED OR WEBGPU_SUPPORTED OR DXC_SUPPORTED)
    set(ENABLE_HLSL TRUE)
endif()

set(ENABLE_WGSL FALSE)
if(WEBGPU_SUPPORTED)
	set(ENABLE_WGSL TRUE)
endif()

if(ENABLE_GLSL)
    list(APPEND SOURCE src/GLSLUtils.cpp)
    list(APPEND INCLUDE include/GLSLUtils.hpp)
endif()

if(ENABLE_HLSL)
    list(APPEND SOURCE src/HLSLUtils.cpp)
    list(APPEND INCLUDE include/HLSLUtils.hpp)
endif()

if(ENABLE_WGSL)
    list(APPEND SOURCE src/WGSLUtils.cpp src/WGSLShaderResources.cpp)
    list(APPEND INCLUDE include/WGSLUtils.hpp include/WGSLShaderResources.hpp)
endif()

if(D3D11_SUPPORTED OR D3D12_SUPPORTED)
    set(DXBC_CHECKSUM_SOURCE
        ../../ThirdParty/GPUOpenShaderUtils/DXBCChecksum.cpp
        ../../ThirdParty/GPUOpenShaderUtils/DXBCChecksum.h
    )
    list(APPEND SOURCE src/DXBCUtils.cpp)
    list(APPEND INCLUDE include/DXBCUtils.hpp)
    list(APPEND INCLUDE include/ResourceBindingMap.hpp)
endif()

if (DXC_SUPPORTED)
    list(APPEND INCLUDE include/DXCompiler.hpp include/DXCompilerLibrary.hpp)
    list(APPEND SOURCE src/DXCompiler.cpp src/DXCompilerLibrary.cpp)

    if(PLATFORM_WIN32)
        list(APPEND SOURCE src/DXCompilerLibraryWin32.cpp)
    elseif(PLATFORM_UNIVERSAL_WINDOWS)
        list(APPEND SOURCE src/DXCompilerLibraryUWP.cpp)
    elseif(PLATFORM_LINUX)
        list(APPEND SOURCE src/DXCompilerLibraryLinux.cpp)
    else()
        message(FATAL_ERROR "Unexpected platform")
    endif()

    if(PLATFORM_LINUX)
        list(APPEND INCLUDE
            ../../ThirdParty/DirectXShaderCompiler/dxc/dxcapi.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/WinAdapter.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/WinFunctions.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/WinIncludes.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/Unicode.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/DxilContainer/DxilContainer.h
            ../../ThirdParty/DirectXShaderCompiler/dxc/DXIL/DxilConstants.h)
        list(APPEND SOURCE
            ../../ThirdParty/DirectXShaderCompiler/dxc/Support/WinAdapter.cpp)
    endif()
else()
    list(APPEND SOURCE src/DXILUtilsStub.cpp)
endif()

set(ENABLE_SPIRV FALSE)
set(USE_GLSLANG FALSE)
set(USE_SPIRV_TOOLS FALSE)
if(DILIGENT_USE_SPIRV_TOOLCHAIN)
    set(ENABLE_SPIRV TRUE)
    # NB: do not use if(TARGET glsang) check here as glsang may be present
    #     (e.g. added by external project), but DILIGENT_NO_GLSLANG may still be set
    if (NOT ${DILIGENT_NO_GLSLANG})
        set(USE_GLSLANG TRUE)
        if (TARGET SPIRV-Tools-opt)
            set(USE_SPIRV_TOOLS TRUE)
        endif()
    endif()
    if (NOT ${DILIGENT_NO_HLSL} AND TARGET SPIRV-Tools-opt)
        set(USE_SPIRV_TOOLS TRUE)
    endif()
endif()

if(ENABLE_SPIRV)
    list(APPEND SOURCE src/SPIRVShaderResources.cpp src/SPIRVUtils.cpp)
    list(APPEND INCLUDE include/SPIRVShaderResources.hpp include/SPIRVUtils.hpp)

    if (${USE_SPIRV_TOOLS})
        list(APPEND SOURCE src/SPIRVTools.cpp)
        list(APPEND INCLUDE include/SPIRVTools.hpp)
    endif()

    if (${USE_GLSLANG})
        list(APPEND SOURCE src/GLSLangUtils.cpp)
        list(APPEND INCLUDE include/GLSLangUtils.hpp)
    endif()
endif()

add_library(Diligent-ShaderTools STATIC ${SOURCE} ${INCLUDE} ${DXBC_CHECKSUM_SOURCE})

target_include_directories(Diligent-ShaderTools
PUBLIC
    include
PRIVATE
    ../GraphicsEngine/include
)

if (DXC_SUPPORTED)
    target_include_directories(Diligent-ShaderTools PUBLIC ../../ThirdParty/DirectXShaderCompiler)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(Diligent-ShaderTools PRIVATE -fms-extensions)
    endif()

    if (NOT PLATFORM_WIN32)
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
            CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_link_libraries(Diligent-ShaderTools PRIVATE dl)
        endif()
    endif()
endif()

target_link_libraries(Diligent-ShaderTools
PRIVATE
    Diligent-BuildSettings
    Diligent-GraphicsAccessories
    Diligent-Common
PUBLIC
    Diligent-GraphicsEngineInterface
)

if (TARGET Diligent-HLSL2GLSLConverterLib AND NOT ${DILIGENT_NO_HLSL})
    target_include_directories(Diligent-ShaderTools PRIVATE ../HLSL2GLSLConverterLib/include)
    target_link_libraries(Diligent-ShaderTools PRIVATE Diligent-HLSL2GLSLConverterLib)
else()
    target_compile_definitions(Diligent-ShaderTools PRIVATE DILIGENT_NO_HLSL=1)
endif()

if(ENABLE_SPIRV)
    target_link_libraries(Diligent-ShaderTools
    PRIVATE
        spirv-cross-core
    )
    if(SPIRV_CROSS_NAMESPACE_OVERRIDE)
        target_compile_definitions(Diligent-ShaderTools PUBLIC DILIGENT_SPIRV_CROSS_NAMESPACE=${SPIRV_CROSS_NAMESPACE_OVERRIDE})
    endif()

    if (${USE_SPIRV_TOOLS})
        target_link_libraries(Diligent-ShaderTools
        PRIVATE
            SPIRV-Tools-opt
        )
        target_compile_definitions(Diligent-ShaderTools PRIVATE USE_SPIRV_TOOLS=1)
    endif()

    if (${USE_GLSLANG})
        target_link_libraries(Diligent-ShaderTools
        PRIVATE
            SPIRV
            glslang
        )
    endif()
endif()

if(ENABLE_WGSL)
    target_link_libraries(Diligent-ShaderTools
    PRIVATE
        libtint
        # We include this library because when building for Emscripten, libtint does not include this library in the dependency list
        tint_lang_wgsl_inspector 
    ) 
endif()

set(HLSL_DEFINITIONS include/HLSLDefinitions.fxh)

set(GENERATED_SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/GeneratedShaders)
file(MAKE_DIRECTORY ${GENERATED_SHADERS_DIR})
target_include_directories(Diligent-ShaderTools PRIVATE ${GENERATED_SHADERS_DIR})

# We must use the full path, otherwise the build system will not be able to properly detect
# changes and shader conversion custom command will run every time
set(HLSL_DEFINITIONS_INC ${GENERATED_SHADERS_DIR}/HLSLDefinitions_inc.fxh)
set_source_files_properties(${HLSL_DEFINITIONS_INC} PROPERTIES GENERATED TRUE)

target_sources(Diligent-ShaderTools PRIVATE
    # A target created in the same directory (CMakeLists.txt file) that specifies any output of the
    # custom command as a source file is given a rule to generate the file using the command at build time.
    ${HLSL_DEFINITIONS_INC}
)

find_package(Python3 REQUIRED)
add_custom_command(OUTPUT ${HLSL_DEFINITIONS_INC} # We must use full path here!
                   COMMAND ${Python3_EXECUTABLE} ${FILE2STRING_PATH} ${HLSL_DEFINITIONS} ${HLSL_DEFINITIONS_INC}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   MAIN_DEPENDENCY ${HLSL_DEFINITIONS}
                   COMMENT "Processing HLSLDefinitions.fxh"
                   VERBATIM
)

set_common_target_properties(Diligent-ShaderTools 17)

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
if (DXBC_CHECKSUM_SOURCE)
    source_group("ThirdParty\\DXBCChecksum" FILES ${DXBC_CHECKSUM_SOURCE})
endif()
source_group("generated" FILES ${HLSL_DEFINITIONS_INC})

set_target_properties(Diligent-ShaderTools PROPERTIES
    FOLDER DiligentCore/Graphics
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-ShaderTools)
endif()
