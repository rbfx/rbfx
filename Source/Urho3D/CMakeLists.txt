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
        install (FILES ${_files} DESTINATION ${DEST_INCLUDE_DIR}/${path})
        # Get source files
        file (${_action} _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${path}/*.cpp)
        list (APPEND SOURCE_FILES ${_files})
    endforeach ()
endmacro()

define_engine_source_files (NORECURSE . Graphics)
define_engine_source_files (Audio Container Core Engine Input IO Math Resource Scene UI)

if (URHO3D_OPENGL)
    define_engine_source_files (Graphics/OpenGL)
elseif (URHO3D_D3D11)
    define_engine_source_files (Graphics/Direct3D11)
elseif (URHO3D_D3D9)
    define_engine_source_files (Graphics/Direct3D9)
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

set (THIRD_PARTY_DEPENDENCIES FreeType JO LZ4 PugiXml rapidjson SDL StanHull STB IconFontCppHeaders Civetweb kNet ik Detour DetourCrowd DetourTileCache Recast Box2D WebP Bullet easy_profiler imgui ImGuizmo LibCpuId GLEW MojoShader fcontext cr yaml-cpp)
foreach (dep ${THIRD_PARTY_DEPENDENCIES})
    if (TARGET "${dep}")
        get_target_property(_TARGET_TYPE ${dep} TYPE)
        if(_TARGET_TYPE STREQUAL "OBJECT_LIBRARY")
            list(APPEND SOURCE_FILES "$<TARGET_OBJECTS:${dep}>")
        endif ()
    endif ()
endforeach()

add_library(Urho3D ${URHO3D_LIBRARY_TYPE} ${SOURCE_FILES} ${HEADER_FILES})
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
        if (${${var}})
            target_compile_definitions(Urho3D PUBLIC -D${var})
        endif ()
    endif ()
endforeach()

if (MINI_URHO)
    target_compile_definitions(Urho3D PUBLIC -DMINI_URHO)
endif ()
target_compile_definitions(Urho3D PRIVATE -DURHO3D_IS_BUILDING)

# Define generated source files
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)
    execute_process (COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h -P ${Urho3D_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_QUIET ERROR_QUIET)
endif ()

# Link dependencies
foreach (dep ${THIRD_PARTY_DEPENDENCIES})
    if (TARGET "${dep}")
        get_target_property(_TARGET_TYPE ${dep} TYPE)
        if(_TARGET_TYPE STREQUAL "OBJECT_LIBRARY")
            target_link_objects(Urho3D "${dep}")
        else ()
            target_link_libraries(Urho3D "${dep}")
        endif ()
    endif ()
endforeach()

if (URHO3D_SSE AND NOT MSVC)
    target_compile_options (Urho3D PUBLIC -msse -msse2)
endif ()

if (URHO3D_TASKS)
    include(CheckIncludeFile)
    check_include_file("valgrind/valgrind.h" HAVE_VALGRIND)
    if (HAVE_VALGRIND EQUAL 1)
        target_compile_definitions(Urho3D PRIVATE -DHAVE_VALGRIND=${HAVE_VALGRIND})
    endif ()
endif ()

if (WIN32)
    target_link_libraries(Urho3D winmm user32 gdi32 winmm imm32 ole32 oleaut32 version uuid)
    if (URHO3D_MINIDUMPS)
        target_link_libraries (Urho3D dbghelp)
    endif ()
elseif (APPLE)
    target_compile_definitions(Urho3D PUBLIC -D_XOPEN_SOURCE=1)
    target_compile_options(Urho3D PUBLIC -stdlib=libc++)
    target_link_libraries (Urho3D iconv "-framework AudioToolbox" "-framework CoreAudio" )
    if (ARM)
        target_link_libraries (Urho3D "-framework AVFoundation" "-framework CoreGraphics" "-framework CoreMotion" "-framework Foundation" "-framework GameController" "-framework OpenGLES" "-framework QuartzCore" "-framework UIKit")
    else ()
        target_link_libraries (Urho3D "-framework Carbon" "-framework Cocoa" "-framework CoreFoundation" "-framework SystemConfiguration" "-framework CoreServices" "-framework CoreVideo" "-framework ForceFeedback" "-framework IOKit" "-framework OpenGL")
    endif ()
elseif (ANDROID)
    target_link_libraries (Urho3D dl log android)
elseif (NOT ANDROID AND NOT ARM AND NOT WEB)
    target_link_libraries (Urho3D dl m rt pthread)
endif ()

# Disable very annoying warning
if (GNU OR Clang)
    target_compile_options(Urho3D PUBLIC -Wno-invalid-offsetof)
endif ()

# Graphics
if (URHO3D_OPENGL)
    if (APPLE)
        # Do nothing
    elseif (WIN32)
        target_link_libraries (Urho3D opengl32)
    elseif (ANDROID OR ARM)
        target_link_libraries (Urho3D GLESv1_CM GLESv2)
    else ()
        target_link_libraries (Urho3D GL)
    endif ()
else ()
    if (URHO3D_D3D9)
        find_package(DirectX REQUIRED D3D9)
    else ()
        find_package(DirectX REQUIRED D3D11)
    endif ()
    target_link_libraries (Urho3D ${DIRECT3D_LIBRARIES})
endif ()

if (BUILD_SHARED_LIBS OR EMSCRIPTEN)
    target_compile_definitions(Urho3D PRIVATE -DURHO3D_EXPORTS)
    # Also add export flags of third party libraries. They are required because those are OBJECT libraries and Urho3D
    # should not try to import from them as if they were DLLs.
    target_compile_definitions(Urho3D
        PRIVATE -D_EXPORTS -DLZ4_DLL_EXPORT=1 -DCIVETWEB_DLL_EXPORTS -D_BUILD_PROFILER
        PUBLIC  -D_IMPORTS -DLZ4_DLL_IMPORT=1 -DCIVETWEB_DLL_IMPORTS
    )
else ()
    target_compile_definitions (Urho3D PUBLIC -DURHO3D_STATIC)
endif ()

# CSharp support
if (URHO3D_CSHARP)
    if (URHO3D_WITH_MONO)
        find_package(Mono REQUIRED)
        target_include_directories(Urho3D PRIVATE ${MONO_INCLUDE_DIRS})
        target_link_libraries(Urho3D ${MONO_LIBRARIES})
        target_compile_options(Urho3D PRIVATE ${MONO_CFLAGS})
        if (MSVC)
            set (MONO_RUNTIME_SHARED ${MONO_PATH}/bin/mono-2.0-sgen.dll)
            if (NOT $ENV{APPVEYOR})
                # TODO: Fails on appveyor for no reason. Repeating command succeeds. WTF?
                add_custom_command (TARGET Urho3D POST_BUILD
                    COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${MONO_RUNTIME_SHARED} ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
                    )
            endif ()
            install (FILES ${MONO_RUNTIME_SHARED} DESTINATION ${DEST_BIN_DIR})
        endif ()
    endif ()
    csharp_bind_target(TARGET Urho3D)
endif ()

# SDK installation
install(TARGETS Urho3D
    EXPORT Urho3D
    LIBRARY DESTINATION ${DEST_LIBRARY_DIR}
    RUNTIME DESTINATION ${DEST_BIN_DIR}
    ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR}
)

# Generate include header with all engine headers
list (REMOVE_ITEM HEADER_FILES Urho3D.h DebugNew.h Precompiled.h)
string (REPLACE ";" ">\n#include <Urho3D/" HEADER_FILES "${HEADER_FILES}")
set (HEADER_FILES "#include <Urho3D/${HEADER_FILES}>")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3DAll.h.in ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h @ONLY)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3DAll.h DESTINATION ${DEST_INCLUDE_DIR})

# Generate import/export header with engine configuration defines
get_property(URHO3D_ENGINE_CONFIG TARGET Urho3D PROPERTY INTERFACE_COMPILE_DEFINITIONS)
list (REMOVE_ITEM URHO3D_ENGINE_CONFIG URHO3D_64BIT URHO3D_SSE URHO3D_IS_BUILDING)
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
