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
        file (${_action} _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.h)
        list (REMOVE_ITEM _files Urho3D.h)
        list (APPEND HEADER_FILES ${_files})
        # Install them
        install (FILES ${_files} DESTINATION ${DEST_INCLUDE_DIR}/${path})
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>> # Urho3D.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>        # For parent projects to find engine headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty> # For accessing includes of some third party libs that do not have 'include' folder with headers (like tracy).
    $<INSTALL_INTERFACE:${DEST_BASE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
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
            target_compile_definitions(Urho3D PUBLIC -D${var})
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
    GLEW
    nativefiledialog
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
                    set_property(TARGET Urho3D APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${dep}")
                elseif (APPLE)
                    set_property(TARGET Urho3D APPEND PROPERTY LINK_LIBRARIES -force_load ${dep})
                endif ()
            elseif (UWP)
                set_property(TARGET Urho3D APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${dep}")
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
            target_include_directories(Urho3D PUBLIC ${VALUE})
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

if (WIN32)
    if (URHO3D_MINIDUMPS OR URHO3D_PROFILING)
        target_link_libraries (Urho3D PUBLIC dbghelp)
    endif ()
    if (UWP)
        target_compile_definitions (Urho3D PUBLIC -DUWP=1)
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

if (URHO3D_PROFILING)
    target_link_libraries(Urho3D PUBLIC Tracy)
endif ()

if (WEB)
    target_link_libraries(Urho3D PUBLIC "-s WASM=$<BOOL:EMSCRIPTEN_WASM>")
    if (URHO3D_THREADING)
        target_compile_options(Urho3D PRIVATE "-s USE_PTHREADS=1")
    endif ()
    set_target_properties(Urho3D PROPERTIES COMPILE_OPTIONS $<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-O3>)
    set_target_properties(Urho3D PROPERTIES LINK_FLAGS_RELEASE -sAGGRESSIVE_VARIABLE_ELIMINATION=1)
    target_compile_options(Urho3D PUBLIC $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:${EMCC_WITH_SOURCE_MAPS_FLAG}>)
    target_link_libraries(Urho3D PUBLIC -sFETCH -sDISABLE_EXCEPTION_CATCHING=1 -sFORCE_FILESYSTEM=1 -sDISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 -lidbfs.js)
    target_link_libraries(Urho3D PUBLIC --bind)
    if (NOT EMSCRIPTEN_TOTAL_MEMORY OR "${EMSCRIPTEN_TOTAL_MEMORY}" STREQUAL "0")
        target_link_libraries(Urho3D PUBLIC "-s ALLOW_MEMORY_GROWTH=1" --no-heap-copy)
    else ()
        math(EXPR EMSCRIPTEN_TOTAL_MEMORY_BYTES "${EMSCRIPTEN_TOTAL_MEMORY} * 1024 * 1024")
        target_link_libraries(Urho3D PUBLIC "-s INITIAL_MEMORY=${EMSCRIPTEN_TOTAL_MEMORY_BYTES}")
    endif ()
    target_link_libraries(Urho3D PUBLIC "-s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s GL_ENABLE_GET_PROC_ADDRESS")
    # TODO: Try to remove FULL_ES3. We only need it for glMapBufferRange and glUnmapBuffer.
    target_link_libraries(Urho3D PUBLIC "-s FULL_ES3=1")
endif ()

if (BUILD_SHARED_LIBS)
    target_compile_definitions(Urho3D
        PRIVATE -DURHO3D_EXPORTS=1 -DAL_EXPORTS=1 -DBOX2D_EXPORTS=1 -DCIVETWEB_DLL_EXPORTS=1 -DEASTL_EXPORTS=1
            -DEASTDC_EXPORTS=1 -DURHO3D_EXPORTS=1 -DFMT_LIB_EXPORT=1 -DGLEW_BUILD=1 -DIMGUI_EXPORTS=1
            -DLZ4_DLL_EXPORT=1 -DNFD_EXPORTS=1 -DPUGIXML_EXPORTS=1 -DSDL_EXPORTS=1 -DTRACY_EXPORTS=1 -DRmlCore_EXPORTS=1
            -DRmlDebugger_EXPORTS=1 -DGLSLANG_EXPORTING=1
    )
else ()
    target_compile_definitions (Urho3D PUBLIC -DURHO3D_STATIC)
endif ()

# Check existence of various header files and their functions required by some of the third-party libraries and Urho3D library
# Set the CMake variables in this scope but only add the compiler defines in the respective library's scope
include (CheckIncludeFile)
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

# Workarounds
if (WEB OR ANDROID)
    if (URHO3D_CXX_STANDARD)
        target_compile_options(Urho3D PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++${URHO3D_CXX_STANDARD}>)
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
        $<INSTALL_INTERFACE:${DEST_INCLUDE_DIR}/CSharp/Swig>
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
        OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Urho3DNet.Scripts
        DEPENDS Urho3DNet
    )

    add_target_csharp(
        TARGET Urho3DNet.CodeGenerator
        PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Urho3DNet.CodeGenerator.csproj
        OUTPUT ${CMAKE_BINARY_DIR}/${DEST_BIN_DIR_CONFIG}/Urho3DNet.CodeGenerator
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

    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/CSharp/Swig DESTINATION ${DEST_INCLUDE_DIR}/CSharp)
    install (FILES ${EXTRA_NATIVE_HEADERS} DESTINATION ${DEST_INCLUDE_DIR}/CSharp/Native)
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
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR_CONFIG}
    RUNTIME DESTINATION ${DEST_BIN_DIR_CONFIG}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG}
)

if (URHO3D_MONOLITHIC_HEADER)
    # Generate include header with all engine headers
    list (REMOVE_ITEM HEADER_FILES Urho3D.h DebugNew.h Precompiled.h WindowsSupport.h Core/Main.h Core/PE.h)
    string (REPLACE ";" ">\n#include <Urho3D/" HEADER_FILES "${HEADER_FILES}")
    set (HEADER_FILES "#include <Urho3D/${HEADER_FILES}>")
    configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3DAll.h.in ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h @ONLY)
    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h DESTINATION ${DEST_INCLUDE_DIR})
endif ()

# Generate import/export header with engine configuration defines
get_property(URHO3D_ENGINE_CONFIG TARGET Urho3D PROPERTY INTERFACE_COMPILE_DEFINITIONS)
list (REMOVE_ITEM URHO3D_ENGINE_CONFIG URHO3D_64BIT)
# TODO: Maybe these should be directly in header?
foreach(HAVE HAVE___SINCOSF HAVE_MALLOC_USABLE_SIZE HAVE_STDINT_H HAVE_INTTYPES_H HAVE_MALLOC_H HAVE_SINCOSF HAVE_RTL_OSVERSIONINFOW)
    if (${${HAVE}})
        list (APPEND URHO3D_ENGINE_CONFIG ${HAVE})
    endif ()
endforeach()
list (REMOVE_DUPLICATES URHO3D_ENGINE_CONFIG)
list (APPEND URHO3D_ENGINE_CONFIG "")   # For last item to be processed correctly.
# Give defines without value a value "1".
string (REPLACE ";" "=1;" URHO3D_ENGINE_CONFIG "${URHO3D_ENGINE_CONFIG}")                   # Append =1 to all defines
string (REGEX REPLACE "(\=[0-9]+)+;" "\\1;" URHO3D_ENGINE_CONFIG "${URHO3D_ENGINE_CONFIG}")   # Clear =1 from defines that already had a value
# Turn list into c++ preprocessor code
string (REGEX REPLACE "([^=]+)=([^;]+);" "#ifndef \\1\n#   define \\1 \\2\n#endif\n" URHO3D_ENGINE_CONFIG "${URHO3D_ENGINE_CONFIG}")
# This will be commented out line in the header.
list (INSERT URHO3D_ENGINE_CONFIG 0 "Engine configuration")
string (REPLACE ";" "\n" URHO3D_ENGINE_CONFIG "${URHO3D_ENGINE_CONFIG}")
file (READ ${CMAKE_CURRENT_SOURCE_DIR}/Urho3D.h URHO3D_H_DATA)
string (REPLACE "@URHO3D_ENGINE_CONFIG@" "${URHO3D_ENGINE_CONFIG}" URHO3D_H_DATA "${URHO3D_H_DATA}")
foreach (config Debug RelWithDebInfo Release MinSizeRel)
    file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${config})
endforeach ()
file (GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/Urho3D.h CONTENT ${URHO3D_H_DATA})
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/Urho3D.h DESTINATION ${DEST_INCLUDE_DIR})
file(GLOB ROOT_HEADERS *.h)
list(REMOVE_ITEM ROOT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/Urho3D.h)
install (FILES ${ROOT_HEADERS} "LinkedPlugins.cpp.in" DESTINATION ${DEST_INCLUDE_DIR})
