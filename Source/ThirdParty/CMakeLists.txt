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
    set (ENABLE_PCH OFF CACHE BOOL "" FORCE)
    set (ENABLE_HLSL OFF CACHE BOOL "" FORCE)
    set (ENABLE_CTEST OFF CACHE BOOL "" FORCE)
    set (ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
    set (BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
    set (SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
    set (BUILD_TESTING OFF CACHE BOOL "" FORCE)
    add_subdirectory(glslang)
    install_third_party_libs (glslang OGLCompiler OSDependent MachineIndependent GenericCodeGen)

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
        #set_target_properties(SPIRV-Tools-shared PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
        install_third_party_libs (SPIRV-Tools-static SPIRV-Tools-opt)
    endif ()
endif ()

if (URHO3D_SYSTEMUI)
    add_subdirectory(imgui)
endif ()

if (URHO3D_NETWORK)
    add_subdirectory (Civetweb)
    add_subdirectory (SLikeNet)
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

if (NOT ANDROID AND NOT ARM AND NOT WEB)
    if (URHO3D_OPENGL)
        add_subdirectory (GLEW)
    endif ()
endif ()

if (URHO3D_PROFILING)
    if (NOT TARGET GLEW AND URHO3D_TOOLS)
        add_subdirectory (GLEW)
    endif ()
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
endif ()

# Tracy's headers are included even in non-profiled builds in order to provide stub profiler macros.
add_subdirectory (tracy)

# When crosscompiling, swig will be built as part of native tools build. See Source/CMakeLists.txt.
add_subdirectory(swig)

vs_group_subdirectory_targets(${CMAKE_CURRENT_SOURCE_DIR} ThirdParty)
