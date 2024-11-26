cmake_minimum_required (VERSION 3.6)

project(Diligent-Archiver CXX)

set(INCLUDE
    include/ArchiverImpl.hpp
    include/SerializationDeviceImpl.hpp
    include/SerializedShaderImpl.hpp
    include/SerializedPipelineStateImpl.hpp
    include/SerializedRenderPassImpl.hpp
    include/SerializedResourceSignatureImpl.hpp
    include/SerializationEngineImplTraits.hpp
)

set(INTERFACE
    interface/Archiver.h
    interface/ArchiverFactory.h
    interface/ArchiverFactoryLoader.h
    interface/SerializationDevice.h
    interface/SerializedPipelineState.h
    interface/SerializedShader.h
)

set(SOURCE
    src/ArchiverImpl.cpp
    src/Archiver_Inc.hpp
    src/ArchiverFactory.cpp
    src/SerializationDeviceImpl.cpp
    src/SerializedShaderImpl.cpp
    src/SerializedPipelineStateImpl.cpp
    src/SerializedResourceSignatureImpl.cpp
    src/SerializedRenderPassImpl.cpp
)

if(D3D11_SUPPORTED)
    list(APPEND SOURCE src/Archiver_D3D11.cpp)
endif()
if(D3D12_SUPPORTED)
    list(APPEND SOURCE src/Archiver_D3D12.cpp)
endif()
if(VULKAN_SUPPORTED)
    list(APPEND SOURCE src/Archiver_Vk.cpp)
endif()
if(GL_SUPPORTED OR GLES_SUPPORTED)
    list(APPEND SOURCE src/Archiver_GL.cpp)
endif()
if(WEBGPU_SUPPORTED)
    list(APPEND SOURCE src/Archiver_WebGPU.cpp)
endif()
if(METAL_SUPPORTED)
    set(MTL_SOURCE src/Archiver_Mtl.mm)
    list(APPEND SOURCE ${MTL_SOURCE})
    
    set_property(SOURCE ${MTL_SOURCE} APPEND_STRING PROPERTY
        # Disable Metal API deprecation warnings such as
        #   'MTLArgumentType' is deprecated: first deprecated in macOS 13.0
        COMPILE_FLAGS " -Wno-deprecated-declarations"
    )
endif()

set(DLL_SOURCE
    src/DLLMain.cpp
    src/Archiver.def
)

add_library(Diligent-ArchiverInterface INTERFACE)
target_link_libraries     (Diligent-ArchiverInterface INTERFACE Diligent-GraphicsEngineInterface)
target_include_directories(Diligent-ArchiverInterface INTERFACE interface)
target_compile_definitions(Diligent-ArchiverInterface INTERFACE ARCHIVER_SUPPORTED=1)

add_library(Diligent-Archiver-static STATIC
    ${SOURCE} ${INTERFACE} ${INCLUDE}
    readme.md
)
add_library(Diligent-Archiver-shared SHARED
    readme.md
)

if(MSVC)
    target_sources(Diligent-Archiver-shared PRIVATE ${DLL_SOURCE})
endif()

target_include_directories(Diligent-Archiver-static
PRIVATE
    include
    ../GraphicsEngine/include
    ../GraphicsEngineD3DBase/include
    ../GraphicsEngineNextGenBase/include
)

target_compile_definitions(Diligent-Archiver-static
PRIVATE
    DILIGENT_NO_GLSLANG=$<BOOL:$<NOT:$<TARGET_EXISTS:glslang>>>
)
if(DEFINED DILIGENT_CORE_COMMIT_HASH)
    target_compile_definitions(Diligent-Archiver-static PRIVATE DILIGENT_CORE_COMMIT_HASH="${DILIGENT_CORE_COMMIT_HASH}")
endif()

target_compile_definitions(Diligent-Archiver-shared PUBLIC ENGINE_DLL=1)


target_link_libraries(Diligent-Archiver-static
PUBLIC
    Diligent-ArchiverInterface
PRIVATE
    Diligent-BuildSettings
    Diligent-PlatformInterface
    Diligent-Common
    Diligent-GraphicsAccessories
    Diligent-ShaderTools
)

if(D3D11_SUPPORTED)
    target_link_libraries(Diligent-Archiver-static PRIVATE Diligent-GraphicsEngineD3D11-static)
    target_include_directories(Diligent-Archiver-static PRIVATE ../GraphicsEngineD3D11/include)
endif()

if(D3D12_SUPPORTED)
    target_link_libraries(Diligent-Archiver-static PRIVATE Diligent-GraphicsEngineD3D12-static)
    target_include_directories(Diligent-Archiver-static PRIVATE ../GraphicsEngineD3D12/include)
endif()

if(VULKAN_SUPPORTED)
    target_link_libraries(Diligent-Archiver-static PRIVATE Diligent-GraphicsEngineVk-static Vulkan::Headers)
    target_include_directories(Diligent-Archiver-static
    PRIVATE
        ../GraphicsEngineVulkan/include
    )
endif()

if(GL_SUPPORTED OR GLES_SUPPORTED)
    target_link_libraries(Diligent-Archiver-static
    PRIVATE
        Diligent-GraphicsEngineOpenGL-static
        spirv-cross-core
        spirv-cross-glsl
    )
    target_include_directories(Diligent-Archiver-static PRIVATE ../GraphicsEngineOpenGL/include)

    if(PLATFORM_WIN32)
        target_link_libraries(Diligent-Archiver-static PRIVATE GLEW::glew opengl32.lib)
    elseif(PLATFORM_LINUX)
        find_package(X11 REQUIRED)
        find_package(OpenGL REQUIRED)
        target_link_libraries(Diligent-Archiver-static PRIVATE GLEW::glew OpenGL::GL X11::X11)
    elseif(PLATFORM_MACOS)
        find_package(OpenGL REQUIRED)
        target_link_libraries(Diligent-Archiver-static PRIVATE GLEW::glew OpenGL::GL)
    elseif(PLATFORM_ANDROID)
        target_link_libraries(Diligent-Archiver-static PRIVATE GLESv3 EGL)
    elseif(PLATFORM_EMSCRIPTEN)
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
endif()

if(METAL_SUPPORTED)
    target_link_libraries(Diligent-Archiver-static
    PRIVATE
    	Diligent-GraphicsEngineMetal-static
        spirv-cross-core
        spirv-cross-msl
        spirv-cross-glsl
   	)
    target_include_directories(Diligent-Archiver-static PRIVATE ../../../DiligentCorePro/Graphics/GraphicsEngineMetal/include)
endif()

if(WEBGPU_SUPPORTED)
    target_link_libraries(Diligent-Archiver-static PRIVATE Diligent-GraphicsEngineWebGPU-static)
    if (NOT PLATFORM_EMSCRIPTEN)
        target_link_libraries(Diligent-Archiver-static PRIVATE webgpu_dawn)
    endif()
    target_include_directories(Diligent-Archiver-static
    PRIVATE
        ../GraphicsEngineWebGPU/include
    )
endif()

target_link_libraries(Diligent-Archiver-shared
PUBLIC
    Diligent-ArchiverInterface
PRIVATE
    Diligent-BuildSettings
    ${WHOLE_ARCHIVE_FLAG} Diligent-Archiver-static ${NO_WHOLE_ARCHIVE_FLAG}
)

if(PLATFORM_WIN32)
    # Do not add 'lib' prefix when building with MinGW
    set_target_properties(Diligent-Archiver-shared PROPERTIES PREFIX "")

    # Set output name to Archiver{32|64}{r|d}
    set_dll_output_name(Diligent-Archiver-shared Archiver)
else()
    set_target_properties(Diligent-Archiver-shared PROPERTIES
        OUTPUT_NAME Archiver
    )
endif()

if (MINGW_BUILD)
    # Restrict export to GetArchiverFactory
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/export.map
        "{ global: *GetArchiverFactory*; local: *; };"
    )
    target_link_options(Diligent-Archiver-shared PRIVATE LINKER:--version-script=export.map)
endif()

source_group("src" FILES ${SOURCE})
source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})
source_group("dll" FILES ${DLL_SOURCE})

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

set_target_properties(Diligent-Archiver-static Diligent-Archiver-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)

set_common_target_properties(Diligent-Archiver-static)
set_common_target_properties(Diligent-Archiver-shared)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-Archiver-shared)
    install_core_lib(Diligent-Archiver-static)
endif()
