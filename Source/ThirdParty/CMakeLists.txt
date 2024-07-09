#
# Copyright (c) 2008-2017 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Third party warnings are irrelevant to us.
if (MSVC)
    ucm_replace_flag("/W[0-3]" "/W0" REGEX)
else ()
    ucm_add_flags("-w")
endif ()

if (UWP)
    add_definitions (-DUWP=1)
endif ()

add_subdirectory(EASTL)
add_subdirectory(LZ4)
add_subdirectory(PugiXml)
add_subdirectory(rapidjson)
add_subdirectory(STB)
add_subdirectory(IconFontCppHeaders)
add_subdirectory(CLI11)
add_subdirectory(fmt)
add_subdirectory(spdlog)
add_subdirectory(SDL)
add_subdirectory(ETCPACK)

if (NOT WEB)
    add_subdirectory(enkiTS)
endif ()

add_subdirectory(tinygltf)
if (URHO3D_TESTING)
    add_subdirectory(catch2)
endif ()
if (URHO3D_TOOLS)
    add_subdirectory(crunch)
endif ()
add_subdirectory(FreeType)
if (URHO3D_RMLUI)
    add_subdirectory(RmlUi)
endif ()

if (URHO3D_GLOW AND DESKTOP)
    add_subdirectory(xatlas)
    set (EMBREE_ISPC_SUPPORT OFF CACHE BOOL "" FORCE)
    set (EMBREE_TUTORIALS OFF CACHE BOOL "" FORCE)
    set (EMBREE_STATIC_LIB ON CACHE BOOL "" FORCE)
    set (EMBREE_TASKING_SYSTEM "INTERNAL" CACHE STRING "" FORCE)
    set (EMBREE_LIBRARY_NAME "embree" CACHE STRING "" FORCE)
    set (EMBREE_STATIC_RUNTIME ${URHO3D_STATIC_RUNTIME} CACHE BOOL "" FORCE)
    set (EMBREE_MAX_ISA "NONE" CACHE STRING "" FORCE)
    set (EMBREE_RAY_MASK ON CACHE BOOL "" FORCE)
    set (EMBREE_ISA_SSE2 ON CACHE BOOL "" FORCE)
    set (EMBREE_ISA_SSE42 OFF CACHE BOOL "" FORCE)
    set (EMBREE_ISA_AVX OFF CACHE BOOL "" FORCE)
    set (EMBREE_ISA_AVX2 OFF CACHE BOOL "" FORCE)
    set (EMBREE_ISA_AVX512KNL OFF CACHE BOOL "" FORCE)
    set (EMBREE_ISA_AVX512SKX OFF CACHE BOOL "" FORCE)
    set (EMBREE_GEOMETRY_CURVE OFF CACHE BOOL "" FORCE)
    set (EMBREE_GEOMETRY_GRID OFF CACHE BOOL "" FORCE)
    set (EMBREE_GEOMETRY_INSTANCE OFF CACHE BOOL "" FORCE)
    set (EMBREE_GEOMETRY_POINT OFF CACHE BOOL "" FORCE)
    set (EMBREE_GEOMETRY_QUAD OFF CACHE BOOL "" FORCE)
    set (EMBREE_GEOMETRY_SUBDIVISION OFF CACHE BOOL "" FORCE)
    set (EMBREE_GEOMETRY_USER OFF CACHE BOOL "" FORCE)
    set (EMBREE_API_NAMESPACE "embree3" CACHE STRING "" FORCE)
    add_subdirectory(embree)
endif ()

if (URHO3D_SHADER_TRANSLATOR)
    set (ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
    set (ENABLE_GLSLANG_INSTALL OFF CACHE BOOL "" FORCE)
    set (ENABLE_PCH OFF CACHE BOOL "" FORCE)
    set (ENABLE_HLSL OFF CACHE BOOL "" FORCE)
    set (ENABLE_CTEST OFF CACHE BOOL "" FORCE)
    set (ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
    set (ENABLE_OPT OFF CACHE BOOL "" FORCE)
    set (BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
    set (SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
    set (BUILD_TESTING OFF CACHE BOOL "" FORCE)
    unset (LLVM_USE_CRT_DEBUG CACHE)
    unset (LLVM_USE_CRT_RELEASE CACHE)
    unset (LLVM_USE_CRT_RELWITHDEBINFO CACHE)
    add_subdirectory(glslang)
    install_third_party_libs (glslang OGLCompiler OSDependent MachineIndependent GenericCodeGen glslang-default-resource-limits)

    set (SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_MSL ON CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_HLSL ON CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_FORCE_PIC ON CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_NAMESPACE_OVERRIDE OFF CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set (SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    add_subdirectory(SPIRV-Cross)
    install_third_party_libs (SPIRV spirv-cross-core spirv-cross-glsl spirv-cross-msl spirv-cross-hlsl)

    if (URHO3D_SHADER_OPTIMIZER)
        add_subdirectory(SPIRV-Headers)
        install_third_party_libs (SPIRV-Headers)

        set (SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
        set (SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
        set (SPIRV_BUILD_COMPRESSION OFF CACHE BOOL "" FORCE)
        set (SPIRV_TOOLS_BUILD_STATIC ON CACHE BOOL "" FORCE)
        set (SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
        set (SPIRV_WERROR OFF CACHE BOOL "" FORCE)
        set (SPIRV_GENERATOR_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Tools-artifacts" CACHE STRING "" FORCE)
        add_subdirectory(SPIRV-Tools)
        install_third_party_libs (SPIRV-Tools-static SPIRV-Tools-opt)
    endif ()
endif ()

if (URHO3D_SYSTEMUI)
    add_subdirectory(imgui)
endif ()

if (URHO3D_NETWORK)
    set(NO_MEDIA ON)
    set(NO_EXAMPLES ON)
    set(NO_TESTS ON)
    set(CMAKE_PROJECT_VERSION 1.0.0)
    if (WEB)
        add_subdirectory(libdatachannel-wasm)
    else ()
        add_subdirectory(openssl)
        add_subdirectory(libdatachannel)
    endif ()
    if (NOT WEB)
        add_subdirectory(Civetweb)
    endif ()
endif ()

if (URHO3D_NAVIGATION)
    add_subdirectory (Detour)
    add_subdirectory (DetourCrowd)
    add_subdirectory (DetourTileCache)
    add_subdirectory (Recast)
endif ()

if (URHO3D_PHYSICS2D)
    add_subdirectory (Box2D)
endif ()

if (URHO3D_WEBP)
    add_subdirectory (WebP)
endif ()

if (URHO3D_PHYSICS)
    add_subdirectory (Bullet)
    add_subdirectory (StanHull)
endif ()

if (URHO3D_TOOLS OR URHO3D_EDITOR)
    add_subdirectory(nativefiledialog)
endif ()

if (NOT UWP)
    add_subdirectory (GLEW)
    add_library (GLEW::glew ALIAS GLEW)
endif ()

if (URHO3D_PROFILING)
    set (CAPSTONE_BUILD_STATIC_RUNTIME ${URHO3D_STATIC_RUNTIME})
    set (CAPSTONE_BUILD_STATIC ON)
    set (CAPSTONE_BUILD_SHARED OFF)
    set (CAPSTONE_BUILD_TESTS OFF)
    set (CAPSTONE_BUILD_CSTOOL OFF)
    set (CAPSTONE_ARCHITECTURE_DEFAULT OFF)
    set (CAPSTONE_X86_SUPPORT ON)
    set (CAPSTONE_ARM_SUPPORT ON)
    set (CAPSTONE_ARM64_SUPPORT ON)
    set (CAPSTONE_X86_ATT_DISABLE ON)
    add_subdirectory (capstone)
    install_third_party_libs (capstone-static capstone-shared)
endif ()

# Tracy's headers are included even in non-profiled builds in order to provide stub profiler macros.
add_subdirectory (tracy)

set (DILIGENT_INSTALL_CORE OFF CACHE BOOL "" FORCE)
set (DILIGENT_NO_GLSLANG ON CACHE BOOL "" FORCE)
set (DILIGENT_NO_HLSL ON CACHE BOOL "" FORCE)
assign_bool (DILIGENT_NO_OPENGL URHO3D_GRAPHICS_NO_GL)
assign_bool (DILIGENT_NO_DIRECT3D11 URHO3D_GRAPHICS_NO_D3D11)
assign_bool (DILIGENT_NO_DIRECT3D12 URHO3D_GRAPHICS_NO_D3D12)
assign_bool (DILIGENT_NO_VULKAN URHO3D_GRAPHICS_NO_VULKAN)
if (NOT URHO3D_DEBUG_GRAPHICS)
    set (DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS "/wd4189" CACHE STRING "" FORCE)
    set (DILIGENT_CLANG_COMPILE_OPTIONS ""  CACHE STRING "" FORCE)
else ()
    set (DILIGENT_MSVC_DEBUG_COMPILE_OPTIONS "" CACHE STRING "" FORCE)
endif ()
add_subdirectory (Diligent)
install_third_party_libs (
    Diligent-AndroidPlatform
    Diligent-ApplePlatform
    Diligent-Archiver-static
    Diligent-ArchiverInterface
    Diligent-BasicPlatform
    Diligent-BuildSettings
    Diligent-Common
    Diligent-EmscriptenPlatform
    Diligent-GraphicsAccessories
    Diligent-GraphicsEngine
    Diligent-GraphicsEngineD3D11-shared
    Diligent-GraphicsEngineD3D11-static
    Diligent-GraphicsEngineD3D11Interface
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineD3D12-static
    Diligent-GraphicsEngineD3D12Interface
    Diligent-GraphicsEngineD3DBase
    Diligent-GraphicsEngineD3DBaseInterface
    Diligent-GraphicsEngineInterface
    Diligent-GraphicsEngineNextGenBase
    Diligent-GraphicsEngineOpenGL-static
    Diligent-GraphicsEngineOpenGLInterface
    Diligent-GraphicsEngineVk-static
    Diligent-GraphicsEngineVkInterface
    Diligent-HLSL2GLSLConverterLib
    Diligent-LinuxPlatform
    Diligent-PlatformInterface
    Diligent-Primitives
    Diligent-PublicBuildSettings
    Diligent-ShaderTools
    Diligent-UniversalWindowsPlatform
    Diligent-Win32Platform
    volk_headers
    Vulkan-Headers
)
if (NOT URHO3D_DEBUG_GRAPHICS)
    get_target_property(DILIGENT_COMPILE_DEFS Diligent-PublicBuildSettings INTERFACE_COMPILE_DEFINITIONS)
    string(REPLACE "$<$<CONFIG:DEBUG>:DILIGENT_DEVELOPMENT;DILIGENT_DEBUG>" "" DILIGENT_COMPILE_DEFS "${DILIGENT_COMPILE_DEFS}")
    set_property(TARGET Diligent-PublicBuildSettings PROPERTY INTERFACE_COMPILE_DEFINITIONS ${DILIGENT_COMPILE_DEFS})
endif ()
file(GLOB_RECURSE DILIGENT_INCLUDE_DIRS LIST_DIRECTORIES true RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/Diligent "Diligent/**")
list(FILTER DILIGENT_INCLUDE_DIRS INCLUDE REGEX ".+/interface$")
foreach(DIR ${DILIGENT_INCLUDE_DIRS})
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Diligent/${DIR} DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/Diligent/${DIR}/..)
endforeach()

# Setup OpenXR after Diligent to use platform defines
if (URHO3D_XR)
    add_definitions(-DDISABLE_STD_FILESYSTEM=1)

    if (UWP)
        set (DYNAMIC_LOADER ON CACHE BOOL "")
        add_definitions(-DDYNAMIC_LOADER=1)
    endif ()
    if (ANDROID)
        set (BUILD_LOADER OFF CACHE BOOL "" FORCE)
    else ()
        set (BUILD_LOADER ON CACHE BOOL "" FORCE)
    endif ()

    add_subdirectory(OpenXRSDK)
    install_third_party_libs(openxr_loader)
endif ()

# When crosscompiling, swig will be built as part of native tools build. See Source/CMakeLists.txt.
add_subdirectory(swig)

vs_group_subdirectory_targets(${CMAKE_CURRENT_SOURCE_DIR} ThirdParty)
