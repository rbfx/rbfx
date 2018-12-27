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
        list (REMOVE_ITEM _files
            Urho3D.h
            Graphics/Direct3D11/D3D11GraphicsImpl.cpp
            Graphics/Direct3D11/D3D11GraphicsImpl.h
            Graphics/Direct3D9/D3D9GraphicsImpl.cpp
            Graphics/Direct3D9/D3D9GraphicsImpl.h
            Graphics/GraphicsImpl.h
            Graphics/OpenGL/OGLGraphicsImpl.h
            IK/IKConverters.h
        )
        list (APPEND HEADER_FILES ${_files})
        # Install them
        if (NOT MINI_URHO)
            install (FILES ${_files} DESTINATION ${DEST_INCLUDE_DIR}/${path})
        endif ()
        # Get source files
        file (${_action} _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.cpp)
        list (APPEND SOURCE_FILES ${_files})
    endforeach ()
endmacro()

define_engine_source_files (Container Core Engine IO Math)

if (NOT MINI_URHO)
    define_engine_source_files (Audio Input Resource Scene UI)
    define_engine_source_files (NORECURSE . Graphics)
    if (URHO3D_OPENGL)
        define_engine_source_files (Graphics/OpenGL)
    elseif (URHO3D_D3D11)
        define_engine_source_files (Graphics/Direct3D11)
    elseif (URHO3D_D3D9)
        define_engine_source_files (Graphics/Direct3D9)
    endif ()
endif ()

if (URHO3D_IK)
    define_engine_source_files (IK)
endif ()

if (URHO3D_NAVIGATION)
    define_engine_source_files (Navigation)
endif ()

if (URHO3D_NETWORK)
    define_engine_source_files (Network)
endif ()

if (URHO3D_PHYSICS)
    define_engine_source_files (Physics)
endif ()

if (URHO3D_URHO2D)
    define_engine_source_files (Urho2D)
endif ()

if (URHO3D_SYSTEMUI)
    define_engine_source_files (SystemUI)
endif ()

if (URHO3D_CSHARP)
    define_engine_source_files (Script)
endif ()

# Group source code in VS solution
group_sources()

if (APPLE)
    list (APPEND SOURCE_FILES IO/MacFileWatcher.m)
endif ()

if (MINI_URHO)
    # Avoid building freetype
    list (REMOVE_ITEM SOURCE_FILES UI/FontFaceFreeType.cpp)
endif ()

add_library(Urho3D ${SOURCE_FILES} ${HEADER_FILES} $<$<CXX_COMPILER_ID:MSVC>:${Urho3D_SOURCE_DIR}/script/Urho3D.natvis>)
target_include_directories(Urho3D PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>        # For parent projects to find auto-generated engine headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>        # For parent projects to find engine headers
    $<INSTALL_INTERFACE:${DEST_BASE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${DEST_THIRDPARTY_HEADERS_DIR}>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# Add any variables starting with URHO3D_ as project defines
get_cmake_property(__cmake_variables VARIABLES)
foreach (var ${__cmake_variables})
    if ("${var}" MATCHES "^URHO3D_")
        if ("${${var}}")
            target_compile_definitions(Urho3D PUBLIC -D${var})
        endif ()
    endif ()
endforeach()

if (MINI_URHO)
    target_compile_definitions(Urho3D PUBLIC -DMINI_URHO)
endif ()
target_compile_definitions(Urho3D PRIVATE -DURHO3D_IS_BUILDING PUBLIC -DNOMINMAX)

# Set platform definitions
foreach(def LINUX ANDROID IOS APPLE MACOS MOBILE DESKTOP)
    if (${def})
        target_compile_definitions(Urho3D PUBLIC -D${def}=1)
    endif ()
endforeach()

# Define generated source files
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)
    execute_process (COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h -P ${Urho3D_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_QUIET ERROR_QUIET)
endif ()

# Link dependencies
set (THIRD_PARTY_DEPENDENCIES
    FreeType
    JO
    LZ4
    PugiXml
    rapidjson
    SDL
    StanHull
    STB
    IconFontCppHeaders
    Civetweb
    SLikeNet
    ik
    Detour
    DetourCrowd
    DetourTileCache
    Recast
    Box2D
    WebP
    Bullet
    ImGui
    LibCpuId
    GLEW
    MojoShader
    sc
    cr
    nativefiledialog
    CLI11
    fmt
)

if (BUILD_SHARED_LIBS AND NOT MSVC AND NOT APPLE)
    target_link_libraries(Urho3D PRIVATE -Wl,--whole-archive)
endif ()

foreach (dep ${THIRD_PARTY_DEPENDENCIES})
    if (TARGET "${dep}")
        # By linking them as private but inheriting definitions/options/dirs we avoid having
        # to export third party lib targets in a monolithic dll build.
        get_target_property(_TARGET_TYPE ${dep} TYPE)
        if (BUILD_SHARED_LIBS AND ${_TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
            if (MSVC)
                set_property(TARGET Urho3D APPEND_STRING PROPERTY LINK_FLAGS " /WHOLEARCHIVE:${dep}")
            elseif (APPLE)
                set_property(TARGET Urho3D APPEND PROPERTY LINK_LIBRARIES -force_load ${dep})
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
                if (NOT TARGET ${dep_lib})
                    target_link_libraries (Urho3D PUBLIC "${dep_lib}")
                endif ()
            endforeach ()
        endif ()
        if (BUILD_SHARED_LIBS)
            target_link_libraries(Urho3D PRIVATE ${dep})
        else ()
            if (${_TARGET_TYPE} STREQUAL "STATIC_LIBRARY")
                list (APPEND DEPENDENCY_TARGET_FILES $<TARGET_FILE:${dep}>)
            endif ()
        endif ()
    endif ()
endforeach()

if (BUILD_SHARED_LIBS AND NOT MSVC AND NOT APPLE)
    target_link_libraries(Urho3D PRIVATE -Wl,--no-whole-archive)
endif ()

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

if (URHO3D_TASKS)
    include(CheckIncludeFile)
    check_include_file("valgrind/valgrind.h" HAVE_VALGRIND)
    if (HAVE_VALGRIND)
        target_compile_definitions(Urho3D PRIVATE -DHAVE_VALGRIND=${HAVE_VALGRIND})
    endif ()
endif ()

if (WIN32)
    if (URHO3D_MINIDUMPS OR URHO3D_PROFILING)
        target_link_libraries (Urho3D PUBLIC dbghelp)
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
    target_compile_options(Urho3D PUBLIC -Wno-invalid-offsetof)
endif ()

if (URHO3D_PROFILING)
    if (TARGET tracy-dll)
        target_link_libraries(Urho3D INTERFACE tracy-dll)
    endif ()
    target_link_libraries(Urho3D PUBLIC tracy-settings)
endif ()

# Graphics
if (URHO3D_OPENGL)
    if (APPLE)
        # Do nothing
    elseif (WIN32)
        target_link_libraries (Urho3D PUBLIC opengl32)
    elseif (ANDROID OR ARM)
        if (URHO3D_GLES2)
            target_link_libraries (Urho3D PUBLIC GLESv1_CM GLESv2)
        elseif (URHO3D_GLES3)
            target_link_libraries (Urho3D PUBLIC GLESv3)
        endif ()
    else ()
        target_link_libraries (Urho3D PUBLIC GL)
    endif ()
else ()
    if (URHO3D_D3D9)
        find_package(DirectX REQUIRED D3D9)
    else ()
        find_package(DirectX REQUIRED D3D11)
    endif ()
    target_link_libraries (Urho3D PUBLIC ${DIRECT3D_LIBRARIES})
endif ()

if (WEB)
    # Switching to WASM may be possible after https://github.com/kripken/emscripten-fastcomp/pull/195 lands and
    # https://github.com/kripken/emscripten/issues/5436 is fixed.
    target_link_libraries(Urho3D PRIVATE "-s EXTRA_EXPORTED_RUNTIME_METHODS=['Pointer_stringify']" PUBLIC "-s WASM=0")
endif ()

if (BUILD_SHARED_LIBS)
    target_compile_definitions(Urho3D PRIVATE -DURHO3D_EXPORTS INTERFACE -DURHO3D_IMPORTS)
else ()
    target_compile_definitions (Urho3D PUBLIC -DURHO3D_STATIC)
endif ()

if (URHO3D_CSHARP)
    add_subdirectory(CSharp)
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
foreach (FUNCT __sincosf malloc_usable_size)
    string (TOUPPER HAVE_${FUNCT} HAVE_FUNCT)
    check_function_exists (${FUNCT} ${HAVE_FUNCT})
endforeach ()
include (CheckLibraryExists)
check_library_exists (m sincosf "" HAVE_SINCOSF)
if (MINGW)
    include (CheckStructHasMember)
    check_struct_has_member (RTL_OSVERSIONINFOW dwOSVersionInfoSize minwindef.h\;winnt.h HAVE_RTL_OSVERSIONINFOW)
endif ()

################################################### SDK preparation ####################################################
if (MINI_URHO)
    return ()
endif ()

if (NOT BUILD_SHARED_LIBS)
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
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)

if (URHO3D_MONOLITHIC_HEADER)
    # Generate include header with all engine headers
    list (REMOVE_ITEM HEADER_FILES Urho3D.h DebugNew.h Precompiled.h Core/Main.h)
    string (REPLACE ";" ">\n#include <Urho3D/" HEADER_FILES "${HEADER_FILES}")
    set (HEADER_FILES "#include <Urho3D/${HEADER_FILES}>")
    configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3DAll.h.in ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h @ONLY)
    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h DESTINATION ${DEST_INCLUDE_DIR})
endif ()

# Generate import/export header with engine configuration defines
get_property(URHO3D_ENGINE_CONFIG TARGET Urho3D PROPERTY INTERFACE_COMPILE_DEFINITIONS)
list (REMOVE_ITEM URHO3D_ENGINE_CONFIG URHO3D_64BIT URHO3D_IS_BUILDING)
# TODO: Maybe these should be directly in header?
foreach(HAVE HAVE___SINCOSF HAVE_MALLOC_USABLE_SIZE HAVE_STDINT_H HAVE_INTTYPES_H HAVE_MALLOC_H HAVE_SINCOSF HAVE_RTL_OSVERSIONINFOW)
    if (${${HAVE}})
        list (APPEND URHO3D_ENGINE_CONFIG ${HAVE})
    endif ()
endforeach()
list (REMOVE_DUPLICATES URHO3D_ENGINE_CONFIG)
list (APPEND URHO3D_ENGINE_CONFIG "")   # For last item to be processed correctly.
# Give defines without value a value "1".
string (REGEX REPLACE "(^|;)([a-zA-Z0-9_]+);" "\\1\\2=1;" URHO3D_ENGINE_CONFIG "${URHO3D_ENGINE_CONFIG}")
# Turn list into c++ preprocessor code
string (REGEX REPLACE "([^=]+)=([^;]+);" "#ifndef \\1\n#   define \\1 \\2\n#endif\n" URHO3D_ENGINE_CONFIG "${URHO3D_ENGINE_CONFIG}")
# This will be commented out line in the header.
list (INSERT URHO3D_ENGINE_CONFIG 0 "Engine configuration")
string (REPLACE ";" "\n" URHO3D_ENGINE_CONFIG "${URHO3D_ENGINE_CONFIG}")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3D.h ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h @ONLY)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.h DESTINATION ${DEST_INCLUDE_DIR})
