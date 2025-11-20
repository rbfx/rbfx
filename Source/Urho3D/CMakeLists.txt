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

project(Urho3D C CXX)

macro (define_engine_source_files)
    cmake_parse_arguments(DEFINE_SRC_FILES "NORECURSE" "" "" ${ARGN} )
    if (DEFINE_SRC_FILES_NORECURSE)
        set (_action GLOB)
    else ()
        set (_action GLOB_RECURSE)
    endif ()
    foreach (path ${DEFINE_SRC_FILES_UNPARSED_ARGUMENTS})
        # Get header files
        file (${_action} _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.h ${path}/*.inl)
        list (REMOVE_ITEM _files Urho3D.h)
        list (APPEND HEADER_FILES ${_files})
        # Install them
        install (FILES ${_files} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urho3D/${path})
        # Get source files
        file (${_action} _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.cpp)
        list (APPEND SOURCE_FILES ${_files})
    endforeach ()
endmacro()

define_engine_source_files (Container Core IO Math)

define_engine_source_files (Audio Engine Graphics Input Plugins RenderAPI RenderPipeline Resource Scene Script Shader UI Utility)
list (REMOVE_ITEM SOURCE_FILES BindAll.cpp)     # Utility file for BindTool.

if (URHO3D_PARTICLE_GRAPH)
    define_engine_source_files (Particles)
endif ()

if (URHO3D_RMLUI)
    define_engine_source_files (RmlUI)
endif ()

if (URHO3D_GLOW)
    define_engine_source_files (Glow)
endif ()

if (URHO3D_IK)
    define_engine_source_files (IK)
endif ()

if (URHO3D_NAVIGATION)
    define_engine_source_files (Navigation)
endif ()

if (URHO3D_NETWORK)
    define_engine_source_files (Network)
    define_engine_source_files (Replica)
endif ()

if (URHO3D_PHYSICS)
    define_engine_source_files (Physics)
endif ()

if (URHO3D_URHO2D)
    define_engine_source_files (Urho2D)
endif ()

if (URHO3D_PHYSICS2D)
    define_engine_source_files (Physics2D)
endif ()

if (URHO3D_SYSTEMUI)
    define_engine_source_files (SystemUI)
endif ()

if (URHO3D_ACTIONS)
    define_engine_source_files (Actions)
endif ()

if (URHO3D_XR)
    add_definitions(-DXR_USE_TIMESPEC)
    if (PLATFORM_WIN32 OR PLATFORM_UNIVERSAL_WINDOWS)
        add_definitions(-DXR_OS_WINDOWS -DXR_USE_PLATFORM_WIN32)
    endif ()
    if (PLATFORM_LINUX)
        add_definitions(-DXR_OS_LINUX -DXR_USE_PLATFORM_LINUX)
    endif ()
    if (PLATFORM_ANDROID)
        add_definitions(-DXR_OS_ANDROID -DXR_USE_PLATFORM_ANDROID)
    endif ()
    if (D3D11_SUPPORTED)
        add_definitions(-DXR_USE_GRAPHICS_API_D3D11)
    endif ()
    if (D3D12_SUPPORTED)
        add_definitions(-DXR_USE_GRAPHICS_API_D3D12)
    endif ()
    if (VULKAN_SUPPORTED)
        add_definitions(-DXR_USE_GRAPHICS_API_VULKAN)
    endif ()
    if (GL_SUPPORTED)
        add_definitions(-DXR_USE_GRAPHICS_API_OPENGL)
    endif ()
    if (GLES_SUPPORTED)
        add_definitions(-DXR_USE_GRAPHICS_API_OPENGL_ES)
    endif ()
    define_engine_source_files (XR)
endif ()
# Group source code in VS solution
group_sources()

if (APPLE AND NOT IOS)
    list (APPEND SOURCE_FILES IO/MacFileWatcher.m)
endif ()

add_library(Urho3D ${SOURCE_FILES} ${HEADER_FILES} $<$<CXX_COMPILER_ID:MSVC>:${rbfx_SOURCE_DIR}/script/rbfx.natvis>)
set_target_properties(Urho3D PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(Urho3D PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>        # For parent projects to find auto-generated engine headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>           # Misc configured headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>        # For parent projects to find engine headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty> # For accessing includes of some third party libs that do not have 'include' folder with headers (like tracy).
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Urho3D>
)

if (URHO3D_PCH)
    target_precompile_headers(Urho3D PRIVATE $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/Precompiled.h>>)
endif ()

# Add any variables starting with URHO3D_ as project defines, with some exceptions
get_cmake_property(__cmake_variables VARIABLES)
set (IGNORED_URHO3D_OPTIONS "URHO3D_PLAYER;URHO3D_EDITOR;URHO3D_EXTRAS;URHO3D_TOOLS;URHO3D_SAMPLES;URHO3D_MERGE_STATIC_LIBS;URHO3D_NO_EDITOR_PLAYER_EXE")
foreach (var ${__cmake_variables})
    list (FIND IGNORED_URHO3D_OPTIONS ${var} _index)
    if ("${var}" MATCHES "^URHO3D_" AND NOT "${var}" MATCHES "_AVAILABLE$" AND _index EQUAL -1)
        if ("${${var}}")
            target_compile_definitions(Urho3D PUBLIC -D${var}=1)
        endif ()
    endif ()
endforeach()

target_compile_definitions(Urho3D PUBLIC -DNOMINMAX -DURHO3D_DEBUG=$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>)

# Set platform definitions
foreach(def LINUX ANDROID IOS APPLE MACOS MOBILE DESKTOP UWP)
    if (${def})
        target_compile_definitions(Urho3D PUBLIC -D${def}=1)
    endif ()
endforeach()

# Define generated source files
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)
    execute_process (COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h -P ${rbfx_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_QUIET ERROR_QUIET)
endif ()

# Public dependencies that are available to the user or are exposed in the public API or headers
set (PUBLIC_THIRD_PARTY_DEPENDENCIES
    FreeType
    LZ4
    PugiXml
    rapidjson
    SDL2-static
    StanHull
    STB
    IconFontCppHeaders
    Civetweb
    Detour
    DetourCrowd
    DetourTileCache
    Recast
    Box2D
    WebP
    ImGui
    ImGuizmo
    GLEW
    nfd
    CLI11
    fmt
    spdlog
    EASTL
    ETCPACK
    xatlas
    embree
    RmlUi
    tinygltf
    enkiTS
    Diligent-Common
    Diligent-GraphicsEngineOpenGL-static
    Diligent-GraphicsEngineVk-static
    Diligent-GraphicsEngineD3D11-static
    Diligent-GraphicsEngineD3D12-static
    Diligent-GraphicsEngineMetal-static
    Diligent-HLSL2GLSLConverterLib
    Vulkan-Headers
    datachannel-static
    TracyClient
)

# Private dependencies that are hidden from the user and are not exposed in the public API or headers
set (PRIVATE_THIRD_PARTY_DEPENDENCIES
    glslang
    glslang-default-resource-limits
    SPIRV
    spirv-cross-core
    spirv-cross-glsl
    spirv-cross-msl
    spirv-cross-hlsl
    SPIRV-Tools
    SPIRV-Tools-opt
    SPIRV-Reflect
)
if (TARGET datachannel-wasm)
    target_link_libraries(Urho3D PUBLIC datachannel-wasm)
endif ()

if (URHO3D_XR AND DESKTOP)
    if (URHO3D_MERGE_STATIC_LIBS)
        list (APPEND PUBLIC_THIRD_PARTY_DEPENDENCIES openxr_loader)
    else () # shouldn't need to wrorry about WEB right now
        target_link_libraries(Urho3D PUBLIC openxr_loader)
    endif ()
endif ()

if (URHO3D_MERGE_STATIC_LIBS)
    list (APPEND PUBLIC_THIRD_PARTY_DEPENDENCIES Bullet)
elseif (TARGET Bullet)
    # This library can not be exported from .dll on windows platforms.
    if (WEB AND BUILD_SHARED_LIBS)
        # With this configuration public linking causes some symbols to be multiply defined.
        target_link_libraries(Urho3D PRIVATE Bullet)
        target_include_directories(Urho3D PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Bullet>)
    else ()
        target_link_libraries(Urho3D PUBLIC Bullet)
    endif ()
endif ()

foreach (dep ${PUBLIC_THIRD_PARTY_DEPENDENCIES})
    if (TARGET "${dep}")
        # By linking them as private but inheriting definitions/options/dirs we avoid having
        # to export third party lib targets in a monolithic dll build.
        get_target_property(_TARGET_TYPE ${dep} TYPE)
        if (${_TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
            if (BUILD_SHARED_LIBS)
                if (MSVC)
                    target_link_options(Urho3D PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${dep}>")
                elseif (APPLE)
                    set_property(TARGET Urho3D APPEND PROPERTY LINK_LIBRARIES -force_load $<TARGET_FILE:${dep}>)
                endif ()
            elseif (UWP)
                target_link_options(Urho3D PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${dep}>")
            endif ()
        endif ()
        get_target_property(VALUE ${dep} INTERFACE_COMPILE_DEFINITIONS)
        if (VALUE)
            target_compile_definitions(Urho3D PUBLIC ${VALUE})
        endif ()
        get_target_property(VALUE ${dep} INTERFACE_COMPILE_OPTIONS)
        if (VALUE)
            target_compile_options(Urho3D PUBLIC ${VALUE})
        endif ()
        get_target_property(VALUE ${dep} INTERFACE_INCLUDE_DIRECTORIES)
        if (VALUE)
            # Process include directories to handle prefix paths properly
            set(PROCESSED_INCLUDES)
            foreach(include_dir ${VALUE})
                # Check if this is an absolute path that starts with a prefix from CMAKE_PREFIX_PATH
                set(FOUND_PREFIX FALSE)
                foreach(prefix ${CMAKE_PREFIX_PATH})
                    if(prefix)
                        # Normalize the prefix path
                        get_filename_component(prefix "${prefix}" ABSOLUTE)
                        # Check if include_dir starts with prefix/include
                        string(FIND "${include_dir}" "${prefix}/${CMAKE_INSTALL_INCLUDEDIR}" PREFIX_POS)
                        if(PREFIX_POS EQUAL 0)
                            # This include path is under a prefix, handle it specially
                            # For BUILD_INTERFACE, use the absolute path
                            # For INSTALL_INTERFACE, remove the prefix
                            string(REPLACE "${prefix}/" "" RELATIVE_INCLUDE "${include_dir}")
                            list(APPEND PROCESSED_INCLUDES
                                "$<BUILD_INTERFACE:${include_dir}>"
                                "$<INSTALL_INTERFACE:${RELATIVE_INCLUDE}>")
                            set(FOUND_PREFIX TRUE)
                            break()
                        endif()
                    endif()
                endforeach()

                # If not under any prefix, just forward as-is
                if(NOT FOUND_PREFIX)
                    list(APPEND PROCESSED_INCLUDES "${include_dir}")
                endif()
            endforeach()

            if(PROCESSED_INCLUDES)
                target_include_directories(Urho3D PUBLIC ${PROCESSED_INCLUDES})
            endif()
        endif ()
        get_target_property(VALUE ${dep} INTERFACE_LINK_LIBRARIES)
        if (VALUE)
            foreach (dep_lib ${VALUE})
                string(FIND "${dep_lib}" "$" DOLLAR_OFFSET)
                if (DOLLAR_OFFSET LESS 0 AND NOT TARGET "${dep_lib}")
                    target_link_libraries (Urho3D PUBLIC "${dep_lib}")
                endif ()
            endforeach ()
        endif ()
        if (BUILD_SHARED_LIBS OR NOT URHO3D_MERGE_STATIC_LIBS)
            if (BUILD_SHARED_LIBS AND NOT MSVC AND NOT APPLE)
                target_link_libraries(Urho3D PRIVATE -Wl,--whole-archive)
            endif ()
            target_link_libraries(Urho3D PRIVATE ${dep})
            if (BUILD_SHARED_LIBS AND NOT MSVC AND NOT APPLE)
                target_link_libraries(Urho3D PRIVATE -Wl,--no-whole-archive)
            endif ()
        else ()
            if (${_TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
                list (APPEND DEPENDENCY_TARGET_FILES $<TARGET_FILE:${dep}>)
            endif ()
        endif ()
    endif ()
endforeach()

# Fix Diligent include directories that are missing from exported targets
# This allows to build the engine with Diligent from edternal ThirdParty
if (TARGET Diligent-Common)
    target_include_directories(Urho3D PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Graphics/GraphicsEngine/interface>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Platforms/interface>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Common/interface>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Primitives/interface>
    )
    # Add graphics API specific include directories
    if (TARGET Diligent-GraphicsEngineD3D11-static)
        target_include_directories(Urho3D PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Graphics/GraphicsEngineD3D11/interface>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Graphics/GraphicsEngineD3DBase/interface>
        )
    endif()
    if (TARGET Diligent-GraphicsEngineD3D12-static)
        target_include_directories(Urho3D PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Graphics/GraphicsEngineD3D12/interface>
        )
    endif()
    if (TARGET Diligent-GraphicsEngineOpenGL-static)
        target_include_directories(Urho3D PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Graphics/GraphicsEngineOpenGL/interface>
        )
    endif()
    if (TARGET Diligent-GraphicsEngineVk-static)
        target_include_directories(Urho3D PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Diligent/Graphics/GraphicsEngineVulkan/interface>
        )
    endif()
endif()

foreach (dep ${PRIVATE_THIRD_PARTY_DEPENDENCIES})
    if (TARGET "${dep}")
        target_link_libraries(Urho3D PRIVATE ${dep})

        get_target_property(_TARGET_TYPE ${dep} TYPE)
        if (URHO3D_MERGE_STATIC_LIBS AND NOT BUILD_SHARED_LIBS AND ${_TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
            list (APPEND DEPENDENCY_TARGET_FILES $<TARGET_FILE:${dep}>)
        endif ()
    endif ()
endforeach ()

if (URHO3D_SSE)
    if (MSVC)
        if (URHO3D_64BIT AND "${URHO3D_SSE}" MATCHES "SSE2?")
            # 64bit instruction set has sse2 included and thus it is always enabled.
            # Specifying this parameter to msvc creates a warning.
        else ()
            target_compile_options (Urho3D PUBLIC /arch:${URHO3D_SSE})
        endif ()
    else ()
        target_compile_options (Urho3D PUBLIC -m${URHO3D_SSE})
    endif ()
    target_compile_definitions (Urho3D PUBLIC -DURHO3D_SSE)
endif ()

if (URHO3D_THREADING OR URHO3D_PROFILING)
    find_package(Threads REQUIRED)
    target_link_libraries(Urho3D PRIVATE Threads::Threads)
endif ()

if (WIN32)
    if (URHO3D_MINIDUMPS OR URHO3D_PROFILING)
        target_link_libraries (Urho3D PUBLIC dbghelp)
    endif ()
    if (UWP)
        target_compile_definitions (Urho3D PUBLIC -DUWP=1)
        # Fix UWP linker error LNK2038: vccorlib must be linked before MSVCRT
        # This ensures proper initialization order of the C++ runtime for UWP applications
        # Link vccorlib library before any other libraries to satisfy UWP requirements
        target_link_libraries(Urho3D PUBLIC $<IF:$<CONFIG:Debug>,vccorlibd,vccorlib>)
    endif ()
    target_link_libraries (Urho3D PUBLIC rpcrt4)
else ()
    target_link_libraries (Urho3D PUBLIC dl)
    if (ANDROID)
        target_link_libraries (Urho3D PUBLIC log android)
        target_compile_definitions (Urho3D PUBLIC -DANDROID_ABI=${ANDROID_ABI})
    elseif (DESKTOP)
        target_link_libraries (Urho3D PUBLIC m pthread)
        if (NOT APPLE)
            target_link_libraries (Urho3D PUBLIC rt uuid)
        endif ()
    endif ()
endif ()

# Disable very annoying warning
if (GNU OR Clang)
    target_compile_options(Urho3D PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>)
endif ()

if (WEB)
    target_link_libraries(Urho3D PUBLIC "-s WASM=$<BOOL:EMSCRIPTEN_WASM>")
    target_compile_options(Urho3D PUBLIC -pthread -sUSE_PTHREADS=1)
    target_compile_options(Urho3D PRIVATE $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-O3>)
    target_link_options(Urho3D PRIVATE $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-sAGGRESSIVE_VARIABLE_ELIMINATION=1>)
    target_compile_options(Urho3D PUBLIC $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:${EMCC_WITH_SOURCE_MAPS_FLAG}>)
    target_link_libraries(Urho3D PUBLIC -sFETCH -sDISABLE_EXCEPTION_CATCHING=1 -sFORCE_FILESYSTEM=1 -sDISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -lidbfs.js)
    target_link_libraries(Urho3D PUBLIC --bind)
    if (NOT EMSCRIPTEN_TOTAL_MEMORY OR "${EMSCRIPTEN_TOTAL_MEMORY}" STREQUAL "0")
        target_link_libraries(Urho3D PUBLIC "-s ALLOW_MEMORY_GROWTH=1" --no-heap-copy)
    else ()
        math(EXPR EMSCRIPTEN_TOTAL_MEMORY_BYTES "${EMSCRIPTEN_TOTAL_MEMORY} * 1024 * 1024")
        target_link_libraries(Urho3D PUBLIC "-s INITIAL_MEMORY=${EMSCRIPTEN_TOTAL_MEMORY_BYTES}")
    endif ()
    target_link_libraries(Urho3D PUBLIC "-s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2")
    if (EMSCRIPTEN_VERSION VERSION_GREATER_EQUAL 3.1.51)
        target_link_libraries(Urho3D PUBLIC -sGL_ENABLE_GET_PROC_ADDRESS=1)
    endif ()
    # TODO: Try to remove FULL_ES3. We only need it for glMapBufferRange and glUnmapBuffer.
    target_link_libraries(Urho3D PUBLIC "-s FULL_ES3=1")
endif ()

# Diligent platform defines.
# This is needed so the user can include Diligent-using headers without linking to Diligent-PublicBuildSettings.
# Platform detection using standard CMake variables
if (EMSCRIPTEN)
    target_compile_definitions(Urho3D PUBLIC -DPLATFORM_EMSCRIPTEN=1)
elseif (ANDROID)
    target_compile_definitions(Urho3D PUBLIC -DPLATFORM_ANDROID=1)
elseif (WIN32)
    if (UWP)
        target_compile_definitions(Urho3D PUBLIC -DPLATFORM_UNIVERSAL_WINDOWS=1)
    else()
        target_compile_definitions(Urho3D PUBLIC -DPLATFORM_WIN32=1)
    endif()
elseif (APPLE)
    if (IOS)
        target_compile_definitions(Urho3D PUBLIC -DPLATFORM_IOS=1)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        target_compile_definitions(Urho3D PUBLIC -DPLATFORM_TVOS=1)
    else()
        target_compile_definitions(Urho3D PUBLIC -DPLATFORM_MACOS=1)
    endif()
elseif (UNIX)
    target_compile_definitions(Urho3D PUBLIC -DPLATFORM_LINUX=1)
endif()

if (BUILD_SHARED_LIBS)
    target_compile_definitions(Urho3D
        PRIVATE -DURHO3D_EXPORTS=1 -DAL_EXPORTS=1 -DBOX2D_EXPORTS=1 -DCIVETWEB_DLL_EXPORTS=1 -DEASTL_EXPORTS=1
            -DEASTDC_EXPORTS=1 -DEAASSERT_EXPORTS=1 -DURHO3D_EXPORTS=1 -DFMT_LIB_EXPORT=1 -DGLEW_BUILD=1 -DIMGUI_EXPORTS=1
            -DLZ4_DLL_EXPORT=1 -DNFD_EXPORT=1 -DPUGIXML_EXPORTS=1 -DSDL_EXPORTS=1 -DRmlCore_EXPORTS=1
            -DRmlDebugger_EXPORTS=1 -DGLSLANG_EXPORTING=1 -DTRACY_EXPORTS=1
    )
else ()
    target_compile_definitions (Urho3D PUBLIC -DURHO3D_STATIC)
endif ()

# Check existence of various header files and their functions required by some of the third-party libraries and Urho3D library
# Set the CMake variables in this scope but only add the compiler defines in the respective library's scope
include (CheckIncludeFile)
include (CheckSymbolExists)
foreach (HEADER stdint.h inttypes.h malloc.h)
    string (TOUPPER HAVE_${HEADER} HAVE_HEADER)
    string (REPLACE . _ HAVE_HEADER ${HAVE_HEADER})
    check_include_file (${HEADER} ${HAVE_HEADER})
endforeach ()
include (CheckFunctionExists)
foreach (FUNCT __sincosf sincosf malloc_usable_size)
    string (TOUPPER HAVE_${FUNCT} HAVE_FUNCT)
    check_symbol_exists (${FUNCT} "math.h;malloc.h" ${HAVE_FUNCT})
endforeach ()
if (MINGW)
    include (CheckStructHasMember)
    check_struct_has_member (RTL_OSVERSIONINFOW dwOSVersionInfoSize minwindef.h\;winnt.h HAVE_RTL_OSVERSIONINFOW)
    if (EXISTS ${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes)
        target_include_directories(Urho3D PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/MinGW-crosscompiler-includes>)
    endif ()
endif ()

if (CLANG AND NOT WEB)
    if (APPLE)
        target_compile_options(Urho3D INTERFACE $<INSTALL_INTERFACE:-stdlib=libc++>)
        target_link_libraries(Urho3D INTERFACE $<INSTALL_INTERFACE:c++ c++abi>)
    else ()
        target_link_libraries(Urho3D INTERFACE $<INSTALL_INTERFACE:stdc++ m>)
    endif ()
endif ()

# Group them together under 'Resources' in Xcode IDE
source_group(Resources FILES ${RESOURCE_FILES})     # RESOURCE_PAKS could be empty if packaging is not requested
target_sources(Urho3D PRIVATE ${RESOURCE_DIRS})

if (URHO3D_CSHARP)
    # Not necessary for build, but required so that users can import Urho3D.i in their bindings .i
    target_include_directories(Urho3D PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Swig>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Urho3D/CSharp/Swig>
    )

    csharp_bind_target(
        TARGET Urho3D
        SWIG ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Swig/Urho3D.i
        CSPROJ ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Urho3DNet.csproj
        NAMESPACE Urho3DNet
    )
    if (URHO3D_SYSTEMUI)
        csharp_bind_target(
            TARGET ImGui
            EMBED Urho3D
            SWIG ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Swig/ImGui.i
            NAMESPACE ImGuiNet
            INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/.."
        )
    endif ()
    unset (CMAKE_SWIG_OUTDIR)

    add_target_csharp(
        TARGET Urho3DNet.Scripts
        PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Urho3DNet.Scripts.csproj
        OUTPUT ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/Urho3DNet.Scripts
        DEPENDS Urho3DNet
    )

    add_target_csharp(
        TARGET Urho3DNet.CodeGenerator
        PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Urho3DNet.CodeGenerator.csproj
        OUTPUT ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/Urho3DNet.CodeGenerator
    )

    file (GLOB_RECURSE EXTRA_NATIVE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Native/*.h)
    file (GLOB_RECURSE EXTRA_NATIVE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Native/*.cpp)
    file (GLOB_RECURSE WRAPPER_FILES)

    target_sources(Urho3D PRIVATE
        ${EXTRA_NATIVE_HEADERS}
        ${EXTRA_NATIVE_SOURCES}
    )

    if (MSVC)
        target_compile_options(Urho3D PRIVATE /bigobj)
    endif ()

    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Swig DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urho3D/CSharp)
    install (FILES ${EXTRA_NATIVE_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urho3D/CSharp/Native)
endif ()

# Install C# dependencies
if (URHO3D_CSHARP)
   install (SCRIPT ${rbfx_SOURCE_DIR}/CMake/Install.cmake)
endif ()

################################################### SDK preparation ####################################################

if (URHO3D_MERGE_STATIC_LIBS)
    # Merge static libraries of third party dependencies to final engine static library
    add_custom_command (TARGET Urho3D PRE_BUILD COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:Urho3D>)
    if (MSVC)
        # If merged library is not deleted in this step then sometimes build system will try to merge dependencies
        # to already merged library inflating it further.
        add_custom_command (TARGET Urho3D POST_BUILD
            COMMAND lib.exe /OUT:$<TARGET_FILE:Urho3D>.engine $<TARGET_FILE:Urho3D> ${DEPENDENCY_TARGET_FILES}
            COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:Urho3D>.engine $<TARGET_FILE:Urho3D>
            COMMENT "Merging all archives into a single static library using lib.exe")
    elseif (APPLE)
        # Apple libtool takes archives directly as input
        add_custom_command (TARGET Urho3D POST_BUILD
            COMMAND libtool -static $<TARGET_FILE:Urho3D> ${DEPENDENCY_TARGET_FILES} -o $<TARGET_FILE:Urho3D>.engine
            COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:Urho3D>.engine $<TARGET_FILE:Urho3D>
            COMMENT "Merging all archives into a single static library using libtool")
    else ()
        # GCC ar does not take archives directly as input like Apple libtool, however, it can be scripted to do so
        add_custom_command (TARGET Urho3D POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rename $<TARGET_FILE:Urho3D> $<TARGET_FILE:Urho3D>.engine
            COMMAND echo CREATE $<TARGET_FILE:Urho3D> >script.ar
            COMMAND echo ADDLIB $<TARGET_FILE:Urho3D>.engine >>script.ar)
        foreach (ARCHIVE ${DEPENDENCY_TARGET_FILES})
            add_custom_command (TARGET Urho3D POST_BUILD COMMAND echo ADDLIB ${ARCHIVE} >>script.ar)
        endforeach ()
        add_custom_command (TARGET Urho3D POST_BUILD
            COMMAND echo SAVE >>script.ar
            COMMAND echo END >>script.ar
            COMMAND ${CMAKE_AR} -M <script.ar
            COMMAND ${CMAKE_COMMAND} -E remove $<TARGET_FILE:Urho3D>.engine script.ar
            COMMENT "Merging all archives into a single static library using ar")
    endif ()
endif ()

# SDK installation
install(TARGETS Urho3D
    EXPORT Urho3D
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if (URHO3D_MONOLITHIC_HEADER)
    # Generate include header with all engine headers
    list (REMOVE_ITEM HEADER_FILES Urho3D.h DebugNew.h Precompiled.h WindowsSupport.h Core/Main.h Core/PE.h)
    list (FILTER HEADER_FILES EXCLUDE REGEX ".*\.inl$")
    string (REPLACE ";" ">\n#include <Urho3D/" HEADER_FILES "${HEADER_FILES}")
    set (HEADER_FILES "#include <Urho3D/${HEADER_FILES}>")
    configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3DAll.h.in ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h @ONLY)
    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urho3D/)
endif ()

file(GLOB ROOT_HEADERS *.h)
install(FILES ${ROOT_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Urho3D/)
install (FILES "LinkedPlugins.cpp.in" "Resources.load.js.in" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
