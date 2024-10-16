cmake_minimum_required (VERSION 3.6)

project(Diligent-GraphicsEngineOpenGL CXX)

set(INCLUDE
    include/AsyncWritableResource.hpp
    include/BufferGLImpl.hpp
    include/BufferViewGLImpl.hpp
    include/DeviceContextGLImpl.hpp
    include/DeviceObjectArchiveGL.hpp
    include/DearchiverGLImpl.hpp
    include/EngineGLImplTraits.hpp
    include/FBOCache.hpp
    include/FenceGLImpl.hpp
    include/FramebufferGLImpl.hpp
    include/GLContext.hpp
    include/GLContextState.hpp
    include/GLObjectWrapper.hpp
    include/GLProgram.hpp
    include/GLProgramCache.hpp
    include/GLStubs.h
    include/GLTypeConversions.hpp
    include/pch.h
    include/PipelineResourceAttribsGL.hpp
    include/PipelineResourceSignatureGLImpl.hpp
    include/PipelineStateGLImpl.hpp
    include/QueryGLImpl.hpp
    include/RenderDeviceGLImpl.hpp
    include/RenderPassGLImpl.hpp
    include/SamplerGLImpl.hpp
    include/ShaderGLImpl.hpp
    include/ShaderResourceBindingGLImpl.hpp
    include/ShaderResourceCacheGL.hpp
    include/ShaderResourcesGL.hpp
    include/ShaderVariableManagerGL.hpp
    include/SwapChainGLBase.hpp
    include/Texture1D_GL.hpp
    include/Texture1DArray_GL.hpp
    include/Texture2D_GL.hpp
    include/Texture2DArray_GL.hpp
    include/Texture3D_GL.hpp
    include/TextureBaseGL.hpp
    include/TextureCube_GL.hpp
    include/TextureCubeArray_GL.hpp
    include/TextureViewGLImpl.hpp
    include/VAOCache.hpp
)

set(INTERFACE
    interface/BaseInterfacesGL.h
    interface/BufferGL.h
    interface/BufferViewGL.h
    interface/DeviceContextGL.h
    interface/EngineFactoryOpenGL.h
    interface/FenceGL.h
    interface/PipelineStateGL.h
    interface/QueryGL.h
    interface/RenderDeviceGL.h
    interface/SamplerGL.h
    interface/ShaderGL.h
    interface/ShaderResourceBindingGL.h
    interface/SwapChainGL.h
    interface/TextureGL.h
    interface/TextureViewGL.h
)

set(SOURCE
    src/BufferGLImpl.cpp
    src/BufferViewGLImpl.cpp
    src/DeviceContextGLImpl.cpp
    src/DeviceObjectArchiveGL.cpp
    src/DearchiverGLImpl.cpp
    src/EngineFactoryOpenGL.cpp
    src/FBOCache.cpp
    src/FenceGLImpl.cpp
    src/FramebufferGLImpl.cpp
    src/GLContextState.cpp
    src/GLObjectWrapper.cpp
    src/GLProgram.cpp
    src/GLProgramCache.cpp
    src/GLTypeConversions.cpp
    src/PipelineResourceSignatureGLImpl.cpp
    src/PipelineStateGLImpl.cpp
    src/QueryGLImpl.cpp
    src/RenderDeviceGLImpl.cpp
    src/RenderPassGLImpl.cpp
    src/SamplerGLImpl.cpp
    src/ShaderGLImpl.cpp
    src/ShaderResourceBindingGLImpl.cpp
    src/ShaderResourceCacheGL.cpp
    src/ShaderResourcesGL.cpp
    src/ShaderVariableManagerGL.cpp
    src/Texture1D_GL.cpp
    src/Texture1DArray_GL.cpp
    src/Texture2D_GL.cpp
    src/Texture2DArray_GL.cpp
    src/Texture3D_GL.cpp
    src/TextureBaseGL.cpp
    src/TextureCube_GL.cpp
    src/TextureCubeArray_GL.cpp
    src/TextureViewGLImpl.cpp
    src/VAOCache.cpp
)

set(DLL_SOURCE
    src/DLLMain.cpp
    src/GraphicsEngineOpenGL.def
)

if(PLATFORM_WIN32)
    list(APPEND SOURCE src/GLContextWindows.cpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)
    list(APPEND INCLUDE include/GLContextWindows.hpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)
elseif(PLATFORM_ANDROID)
    list(APPEND SOURCE src/GLContextAndroid.cpp)
    list(APPEND SOURCE src/RenderDeviceGLESImpl.cpp)
    list(APPEND SOURCE src/GLStubsAndroid.cpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)

    list(APPEND INCLUDE include/GLContextAndroid.hpp)
    list(APPEND INCLUDE include/GLStubsAndroid.h)
    list(APPEND INCLUDE include/RenderDeviceGLESImpl.hpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)

    list(APPEND INTERFACE interface/RenderDeviceGLES.h)
elseif(PLATFORM_LINUX)
    list(APPEND SOURCE src/GLContextLinux.cpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)
    list(APPEND INCLUDE include/GLContextLinux.hpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)
elseif(PLATFORM_MACOS)
    list(APPEND SOURCE src/GLContextMacOS.mm)
    list(APPEND INCLUDE include/GLContextMacOS.hpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)
elseif(PLATFORM_IOS)
    list(APPEND SOURCE src/GLContextIOS.mm)
    list(APPEND INCLUDE include/GLContextIOS.hpp)
    list(APPEND INCLUDE include/GLStubsIOS.h)
    list(APPEND SOURCE src/SwapChainGLIOS.mm)
    list(APPEND INCLUDE include/SwapChainGLIOS.hpp)
elseif(PLATFORM_EMSCRIPTEN)
    list(APPEND SOURCE src/GLContextEmscripten.cpp)
    list(APPEND SOURCE src/RenderDeviceGLESImpl.cpp)
    list(APPEND SOURCE src/SwapChainGLImpl.cpp)

    list(APPEND INCLUDE include/GLContextEmscripten.hpp)
    list(APPEND INCLUDE include/GLStubsEmscripten.h)
    list(APPEND INCLUDE include/RenderDeviceGLESImpl.hpp)
    list(APPEND INCLUDE include/SwapChainGLImpl.hpp)

    list(APPEND INTERFACE interface/RenderDeviceGLES.h)
else()
    message(FATAL_ERROR "Unknown platform")
endif()

add_library(Diligent-GraphicsEngineOpenGLInterface INTERFACE)
target_link_libraries     (Diligent-GraphicsEngineOpenGLInterface INTERFACE Diligent-GraphicsEngineInterface)
target_include_directories(Diligent-GraphicsEngineOpenGLInterface INTERFACE interface)


add_library(Diligent-GraphicsEngineOpenGL-static STATIC
    ${SOURCE} ${INTERFACE} ${INCLUDE}
    readme.md
)

add_library(Diligent-GraphicsEngineOpenGL-shared SHARED
    readme.md
)
if(MSVC)
    target_sources(Diligent-GraphicsEngineOpenGL-shared PRIVATE ${DLL_SOURCE})
endif()

target_include_directories(Diligent-GraphicsEngineOpenGL-static
PRIVATE
    include
)

set(PRIVATE_DEPENDENCIES
    Diligent-BuildSettings
    Diligent-Common
    Diligent-TargetPlatform
    Diligent-GraphicsEngine
    Diligent-ShaderTools
)

if(TARGET Diligent-HLSL2GLSLConverterLib AND NOT ${DILIGENT_NO_HLSL})
    target_include_directories(Diligent-GraphicsEngineOpenGL-static PRIVATE ../HLSL2GLSLConverterLib/include)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} Diligent-HLSL2GLSLConverterLib)
else()
    message("HLSL support is disabled. OpenGL backend will not be able to consume HLSL source.")
    target_compile_definitions(Diligent-GraphicsEngineOpenGL-static PRIVATE DILIGENT_NO_HLSL=1)
endif()

set(PUBLIC_DEPENDENCIES
    Diligent-GraphicsEngineOpenGLInterface
)

if(PLATFORM_WIN32)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} GLEW::glew opengl32.lib)
elseif(PLATFORM_ANDROID)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} GLESv3 EGL)
elseif(PLATFORM_LINUX)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} GLEW::glew OpenGL::GL X11::X11)
elseif(PLATFORM_MACOS)
    find_package(OpenGL REQUIRED)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} GLEW::glew OpenGL::GL)
elseif(PLATFORM_IOS)
    set(PRIVATE_DEPENDENCIES ${PRIVATE_DEPENDENCIES} ${OPENGLES})
elseif(PLATFORM_EMSCRIPTEN)
else()
    message(FATAL_ERROR "Unknown platform")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # WARNING: set_source_files_properties() does not append
    #          properties, but overwrites them!
    # Disable warnings like this one:
    #   comparison of function 'glPolygonMode' not equal to a null pointer is always true [-Wtautological-pointer-compare]
    set_property(SOURCE
        src/RenderDeviceGLImpl.cpp
        src/GLContextState.cpp
        src/DeviceContextGLImpl.cpp
    APPEND_STRING PROPERTY
        COMPILE_FLAGS " -Wno-tautological-pointer-compare"
    )

    if (PLATFORM_IOS)
        # Disable warnings like this one:
        #   unused variable 'BottomLeftY' [-Wunused-variable]
        set_property(SOURCE
            src/DeviceContextGLImpl.cpp
            src/GLContextState.cpp
            src/RenderDeviceGLImpl.cpp
            src/Texture1D_GL.cpp
        APPEND_STRING PROPERTY
            COMPILE_FLAGS " -Wno-unused-variable"
        )
    endif()
endif()

if (MINGW_BUILD)
    # Restrict export to GetEngineFactoryOpenGL
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/export.map
        "{ global: *GetEngineFactoryOpenGL*; local: *; };"
    )
    target_link_options(Diligent-GraphicsEngineOpenGL-shared PRIVATE LINKER:--version-script=export.map)
endif()

target_link_libraries(Diligent-GraphicsEngineOpenGL-static
PRIVATE
    ${PRIVATE_DEPENDENCIES}
PUBLIC
    ${PUBLIC_DEPENDENCIES}
)
target_link_libraries(Diligent-GraphicsEngineOpenGL-shared
PRIVATE
    Diligent-BuildSettings
    ${WHOLE_ARCHIVE_FLAG} Diligent-GraphicsEngineOpenGL-static ${NO_WHOLE_ARCHIVE_FLAG}
PUBLIC
    ${PUBLIC_DEPENDENCIES}
)

target_compile_definitions(Diligent-GraphicsEngineOpenGL-shared PUBLIC ENGINE_DLL=1)
if(PLATFORM_MACOS)
    # Silence OpenGL deprecation warnings
    target_compile_definitions(Diligent-GraphicsEngineOpenGL-static PUBLIC GL_SILENCE_DEPRECATION)
elseif(PLATFORM_IOS)
    # Silence GLES deprecation warnings
    target_compile_definitions(Diligent-GraphicsEngineOpenGL-static PUBLIC GLES_SILENCE_DEPRECATION)
elseif(PLATFORM_EMSCRIPTEN)
    target_link_options(Diligent-GraphicsEngineOpenGL-static PUBLIC "SHELL: -s FULL_ES3=1 -s USE_PTHREADS=1")
endif()

if(PLATFORM_WIN32)
    # Do not add 'lib' prefix when building with MinGW
    set_target_properties(Diligent-GraphicsEngineOpenGL-shared PROPERTIES PREFIX "")

    # Set output name to GraphicsEngineOpenGL_{32|64}{r|d}
    set_dll_output_name(Diligent-GraphicsEngineOpenGL-shared GraphicsEngineOpenGL)
else()
    set_target_properties(Diligent-GraphicsEngineOpenGL-shared PROPERTIES
        OUTPUT_NAME GraphicsEngineOpenGL
    )
endif()

add_library(Diligent-GLAdapterSelector OBJECT "src/GLAdapterSelector.cpp")
target_link_libraries(Diligent-GLAdapterSelector PRIVATE Diligent-BuildSettings)

set_common_target_properties(Diligent-GraphicsEngineOpenGL-shared)
set_common_target_properties(Diligent-GraphicsEngineOpenGL-static)
set_common_target_properties(Diligent-GLAdapterSelector)

source_group("src" FILES ${SOURCE})
if(PLATFORM_WIN32)
    source_group("dll" FILES ${DLL_SOURCE})
endif()

source_group("include" FILES ${INCLUDE})
source_group("interface" FILES ${INTERFACE})

set_target_properties(Diligent-GraphicsEngineOpenGL-static PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GraphicsEngineOpenGL-shared PROPERTIES
    FOLDER DiligentCore/Graphics
)
set_target_properties(Diligent-GLAdapterSelector PROPERTIES 
    FOLDER DiligentCore/Graphics
)

set_source_files_properties(
    readme.md PROPERTIES HEADER_FILE_ONLY TRUE
)

if(DILIGENT_INSTALL_CORE)
    install_core_lib(Diligent-GraphicsEngineOpenGL-shared)
    install_core_lib(Diligent-GraphicsEngineOpenGL-static)
    install_core_lib(Diligent-GLAdapterSelector)
endif()
