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

# Configure cache directories
if (NOT "$ENV{CPM_BINARY_CACHE_DIR}")
    set(CPM_BINARY_CACHE_DIR ${CMAKE_BINARY_DIR}/../rbfx-deps-cache)
    get_filename_component(CPM_BINARY_CACHE_DIR ${CPM_BINARY_CACHE_DIR} REALPATH)
endif()
if (NOT "$ENV{CPM_SOURCE_CACHE}")
    set(CPM_SOURCE_CACHE ${CPM_BINARY_CACHE_DIR}/src)
    get_filename_component(CPM_BINARY_CACHE_DIR ${CPM_BINARY_CACHE_DIR} REALPATH)
endif()
include(${rbfx_SOURCE_DIR}/CMake/Modules/CPM.Cached.cmake)

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

# fmt
CPMAddPackageCached(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 10.2.0
  EXCLUDE_FROM_ALL ON
  OPTIONS "BUILD_SHARED_LIBS OFF"
          "FMT_DOC OFF"
          "FMT_TEST OFF"
          "CMAKE_CXX_FLAGS -DFMT_LIB_EXPORT=1"
)
list(APPEND RBFX_DLL_EXPORT_DEFINES "-DFMT_LIB_EXPORT=1")
list(APPEND RBFX_DLL_IMPORT_DEFINES "-DFMT_SHARED=1")

# spdlog
CPMAddPackageCached(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  VERSION 1.13.0
  EXCLUDE_FROM_ALL ON
  OPTIONS "BUILD_SHARED_LIBS OFF"
          "SPDLOG_FMT_EXTERNAL ON"
          "SPDLOG_DISABLE_DEFAULT_LOGGER ON"
)

# SDL
CPMAddPackageCached(
  NAME SDL2
  GITHUB_REPOSITORY libsdl-org/SDL
  GIT_TAG release-2.24.0
  EXCLUDE_FROM_ALL ON
  OPTIONS "BUILD_SHARED_LIBS OFF"
          "IOS ${IOS}"
          "UWP ${UWP}"
          "ANDROID ${ANDROID}"
          "WEB ${WEB}"
          "SDL_RENDER_D3D OFF"
          "SDL_RENDER_METAL OFF"
          "SDL_TEST OFF"
          "SDL2_DISABLE_SDL2MAIN ON"
          "SDL2_DISABLE_INSTALL OFF"
          "CMAKE_C_FLAGS -DSDL_EXPORTS=1"
          "CMAKE_CXX_FLAGS -DSDL_EXPORTS=1"
  PATCHES ${CMAKE_CURRENT_SOURCE_DIR}/SDL.patch
)
list(APPEND RBFX_DLL_EXPORT_DEFINES "-DSDL_EXPORTS=1")
list(APPEND RBFX_DLL_IMPORT_DEFINES "-DSDL_IMPORTS=1")

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
    add_subdirectory(ImGui)
    add_subdirectory(ImGuizmo)
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

if (DESKTOP AND (URHO3D_PROFILING OR URHO3D_TOOLS OR URHO3D_EDITOR))
    set(NFD_PORTAL ON)
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
    set (CAPSTONE_INSTALL OFF)
    set (CAPSTONE_ARCHITECTURE_DEFAULT OFF)
    set (CAPSTONE_X86_SUPPORT ON)
    set (CAPSTONE_ARM_SUPPORT ON)
    set (CAPSTONE_ARM64_SUPPORT ON)
    set (CAPSTONE_X86_ATT_DISABLE ON)
    add_subdirectory (capstone)
    install_third_party_libs (capstone)
endif ()

if (URHO3D_PROFILING)
    set (TRACY_ON_DEMAND ON)
    add_subdirectory(tracy)

    if (DESKTOP)
        add_library(TracyImGui ALIAS ImGui)
        if (TARGET nfd)
            add_library(TracyNfd ALIAS nfd)
        endif ()
        find_package(SDL2 REQUIRED)
        add_library(TracyGlfw3 INTERFACE)
        target_link_libraries(TracyGlfw3 INTERFACE SDL2::SDL2-static)
        add_library(TracyFreetype ALIAS FreeType)
        add_library(TracyCapstone ALIAS capstone)

        set(LEGACY ON)
        set(NO_ISA_EXTENSIONS ON)
        set(NO_PARALLEL_STL ON)
        set(NO_FILESELECTOR ON)
        add_subdirectory(tracy/profiler)

        install (TARGETS tracy-profiler RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG})
    endif ()
else ()
    add_library(TracyClient INTERFACE)
    target_include_directories(TracyClient INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tracy/public>)
endif ()

# Tracy's headers are included even in non-profiled builds in order to provide stub profiler macros.
install_third_party_libs(TracyClient)
target_include_directories(TracyClient SYSTEM INTERFACE $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}/tracy>)

foreach (dir common client tracy libbacktrace)
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tracy/public/${dir} DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/tracy/
        PATTERN ".+\\.h(pp)?$"
        PATTERN "TracyClient\\.cpp" EXCLUDE
        PATTERN ".+/tracy/Tracy(OpenCL|Vulkan).hpp" EXCLUDE)
endforeach ()

set (DILIGENT_INSTALL_CORE OFF CACHE BOOL "" FORCE)
set (DILIGENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (DILIGENT_NO_GLSLANG ON CACHE BOOL "" FORCE)
set (DILIGENT_NO_HLSL ON CACHE BOOL "" FORCE)
set (DILIGENT_NO_ARCHIVER ON CACHE BOOL "" FORCE)
set (DILIGENT_NO_WEBGPU ON CACHE BOOL "" FORCE)
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
add_subdirectory (Diligent EXCLUDE_FROM_ALL)
set (DILIGENT_ALL_TARGETS
    Diligent-AndroidPlatform
    Diligent-ApplePlatform
    Diligent-BasicPlatform
    Diligent-BuildSettings
    Diligent-Common
    Diligent-EmscriptenPlatform
    Diligent-GraphicsAccessories
    Diligent-GraphicsEngine
    Diligent-GraphicsEngineD3D11-static
    Diligent-GraphicsEngineD3D11Interface
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
    # Diligent creates this target manually instead of using 3rd-party CMake project, so it also has to be patched
    Vulkan-Headers
)
install_third_party_libs (${DILIGENT_ALL_TARGETS})
install_third_party_libs (volk_headers)

# Patch: Disable Diligent debug asserts in debug build unless requested.
if (NOT URHO3D_DEBUG_GRAPHICS)
    get_target_property(DILIGENT_COMPILE_DEFS Diligent-PublicBuildSettings INTERFACE_COMPILE_DEFINITIONS)
    string(REPLACE "$<$<CONFIG:DEBUG>:DILIGENT_DEVELOPMENT;DILIGENT_DEBUG>" "" DILIGENT_COMPILE_DEFS "${DILIGENT_COMPILE_DEFS}")
    set_property(TARGET Diligent-PublicBuildSettings PROPERTY INTERFACE_COMPILE_DEFINITIONS ${DILIGENT_COMPILE_DEFS})
endif ()

# Patch: Diligent does not use install interface, make it explicitly empty
foreach (TARGET ${DILIGENT_ALL_TARGETS})
    if (TARGET ${TARGET})
        get_target_property (DIRS ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)
        if (DIRS)
            set_target_properties (${TARGET} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${DIRS}>")
        endif ()
    endif ()
endforeach ()

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

    add_subdirectory(OpenXRSDK EXCLUDE_FROM_ALL)
    install_third_party_libs(openxr_loader)

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/OpenXRSDK/include/openxr DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR})
    target_include_directories(openxr_loader INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/OpenXRSDK/include>)
endif ()

# When crosscompiling, swig will be built as part of native tools build. See Source/CMakeLists.txt.
add_subdirectory(swig)

vs_group_subdirectory_targets(${CMAKE_CURRENT_SOURCE_DIR} ThirdParty)

set_property_recursive(. POSITION_INDEPENDENT_CODE ON)

# Pass these defines to Urho3D target so dependencies can be exported from the main dll.
set(RBFX_DLL_EXPORT_DEFINES "${RBFX_DLL_EXPORT_DEFINES}" PARENT_SCOPE)
set(RBFX_DLL_IMPORT_DEFINES "${RBFX_DLL_IMPORT_DEFINES}" PARENT_SCOPE)

# CPM may have modified this variable, expose CPM packages to our other projects.
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
