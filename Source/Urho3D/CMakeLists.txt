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

file (GLOB SOURCE_FILES
    *.cpp *.h
    Graphics/*.cpp Graphics/*.h
)
file (GLOB_RECURSE SOURCE_FILES_EXT
    Audio/*.cpp Audio/*.h
    Container/*.cpp Container/*.h
    Core/*.cpp Core/*.h
    Engine/*.cpp Engine/*.h
    Input/*.cpp Input/*.h
    IO/*.cpp IO/*.h
    Math/*.cpp Math/*.h
    Resource/*.cpp Resource/*.h
    Scene/*.cpp Scene/*.h
    UI/*.cpp UI/*.h
)

install (
    DIRECTORY Audio Container Core Engine Input IO Math Resource Scene UI
    DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
)

if (URHO3D_D3D11)
    file (GLOB SOURCE_FILES_GRAPHICS Graphics/Direct3D11/*.h Graphics/Direct3D11/*.cpp)
    install (
        DIRECTORY Graphics/Direct3D11
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
elseif (URHO3D_D3D9)
    file (GLOB SOURCE_FILES_GRAPHICS Graphics/Direct3D9/*.h Graphics/Direct3D9/*.cpp)
    install (
        DIRECTORY Graphics/Direct3D9
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
elseif (URHO3D_OPENGL)
    file (GLOB SOURCE_FILES_GRAPHICS Graphics/OpenGL/*.h Graphics/OpenGL/*.cpp)
    install (
        DIRECTORY Graphics/OpenGL
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_DATABASE_ODBC OR URHO3D_DATABASE_SQLITE)
    file (GLOB SOURCE_FILES_DATABASE Database/*.h Database/*.cpp)
    install (
        DIRECTORY Database
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_DATABASE_ODBC)
    file (GLOB SOURCE_FILES_DATABASE_IMPL Database/ODBC/*.h Database/ODBC/*.cpp)
    install (
        DIRECTORY Database/ODBC
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_DATABASE_SQLITE)
    file (GLOB SOURCE_FILES_DATABASE_IMPL Database/SQLite/*.h Database/SQLite/*.cpp)
    install (
        DIRECTORY Database/SQLite
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_IK)
    file (GLOB_RECURSE SOURCE_FILES_IK IK/*.h IK/*.cpp)
    install (
        DIRECTORY IK
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_NAVIGATION)
    file (GLOB_RECURSE SOURCE_FILES_NAVIGATION Navigation/*.h Navigation/*.cpp)
    install (
        DIRECTORY Navigation
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_NETWORK)
    file (GLOB_RECURSE SOURCE_FILES_NETWORK Network/*.h Network/*.cpp)
    install (
        DIRECTORY Network
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_PHYSICS)
    file (GLOB_RECURSE SOURCE_FILES_PHYSICS Physics/*.h Physics/*.cpp)
    install (
        DIRECTORY Physics
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

if (URHO3D_URHO2D)
    file (GLOB_RECURSE SOURCE_FILES_URHO2D Urho2D/*.h Urho2D/*.cpp)
    install (
        DIRECTORY Urho2D
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

add_library(Urho3D
    ${SOURCE_FILES}
    ${SOURCE_FILES_EXT}
    ${SOURCE_FILES_GRAPHICS}
    ${SOURCE_FILES_DATABASE}
    ${SOURCE_FILES_DATABASE_IMPL}
    ${SOURCE_FILES_IK}
    ${SOURCE_FILES_NAVIGATION}
    ${SOURCE_FILES_NETWORK}
    ${SOURCE_FILES_PHYSICS}
    ${SOURCE_FILES_PHYSICS}
    ${SOURCE_FILES_URHO2D}
)
target_include_directories(Urho3D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Add any variables starting with URHO3D_ as project defines
get_cmake_property(__cmake_variables VARIABLES)
foreach (var ${__cmake_variables})
    if ("${var}" MATCHES "^URHO3D_")
        if (${${var}})
            target_compile_definitions(Urho3D PUBLIC -D${var})
        endif ()
    endif ()
endforeach()

# Define generated source files
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/librevision.h)
    execute_process (COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h -P CMake/Modules/GetUrhoRevision.cmake
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_QUIET ERROR_QUIET)
endif ()
target_include_directories(Urho3D PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Link dependencies
target_link_libraries(Urho3D FreeType JO LZ4 PugiXml rapidjson SDL StanHull STB Mustache imgui)

if (URHO3D_NETWORK)
    target_link_libraries(Urho3D Civetweb kNet)
endif ()

if (URHO3D_DATABASE_ODBC)
    target_link_libraries(Urho3D nanodbc)
endif ()

if (URHO3D_DATABASE_SQLITE)
    target_link_libraries(Urho3D sqlite)
endif ()

if (URHO3D_IK)
    target_link_libraries(Urho3D ik)
endif ()

if (URHO3D_NAVIGATION)
    target_link_libraries(Urho3D Detour DetourCrowd DetourTileCache Recast)
endif ()

if (URHO3D_URHO2D)
    target_link_libraries(Urho3D Box2D)
endif ()

if (URHO3D_WEBP)
    target_link_libraries(Urho3D WebP)
endif ()

if (URHO3D_PHYSICS)
    target_link_libraries(Urho3D Bullet)
endif ()

if (URHO3D_PROFILING)
    target_link_libraries(Urho3D easy_profiler)
endif ()


if (WIN32)
    target_link_libraries(Urho3D LibCpuId)
    target_link_libraries(Urho3D winmm user32 gdi32 winmm imm32 ole32 oleaut32 version uuid)
    if (URHO3D_MINIDUMPS)
        target_link_libraries (Urho3D dbghelp)
    endif ()
elseif (APPLE)
    target_link_libraries (Urho3D iconv)
    if (ARM)
        target_link_libraries (Urho3D "-framework AudioToolbox" "-framework AVFoundation" "-framework CoreAudio" "-framework CoreGraphics" "-framework CoreMotion" "-framework Foundation" "-framework GameController" "-framework OpenGLES" "-framework QuartzCore" "-framework UIKit")
    else ()
        target_link_libraries (Urho3D "-framework AudioToolbox" "-framework Carbon" "-framework Cocoa" "-framework CoreFoundation" "-framework SystemConfiguration" "-framework CoreAudio" "-framework CoreServices" "-framework CoreVideo" "-framework ForceFeedback" "-framework IOKit" "-framework OpenGL")
    endif ()
elseif (ANDROID)
    target_link_libraries (Urho3D dl log android)
elseif (NOT ANDROID AND NOT ARM AND NOT WEB)
    target_link_libraries (Urho3D dl m rt)
endif ()

# Disable very annoying warning
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(Urho3D PRIVATE -Wno-invalid-offsetof)
endif ()

# Graphics
if (URHO3D_D3D9)
    target_link_libraries(Urho3D MojoShader)
elseif (URHO3D_OPENGL)
    target_link_libraries(Urho3D GLEW)
    if (APPLE)
        # Do nothing
    elseif (WIN32)
        target_link_libraries (Urho3D opengl32)
    elseif (ANDROID OR ARM)
        target_link_libraries (Urho3D GLESv1_CM GLESv2)
    else ()
        target_link_libraries (Urho3D GL)
    endif ()
elseif (DIRECT3D_LIBRARIES)
    target_link_libraries (Urho3D ${DIRECT3D_LIBRARIES})
endif ()

target_include_directories(Urho3D PUBLIC ${CMAKE_BINARY_DIR}/${DEST_THIRDPARTY_HEADERS})

if (BUILD_SHARED_LIBS)
    set (INSTALL_TARGET Urho3D)
else ()

    get_property(URHO3D_INTERFACE_LINK_LIBRARIES TARGET Urho3D PROPERTY INTERFACE_LINK_LIBRARIES)
    foreach (lib ${URHO3D_INTERFACE_LINK_LIBRARIES})
        if (TARGET ${lib})
            list (APPEND URHO3D_LINK_TARGETS ${lib})
        endif ()
    endforeach ()

    set_target_properties(Urho3D PROPERTIES OUTPUT_NAME Urho3DBase)
    merge_libraries(TARGET Urho3D_M STATIC Urho3D ${URHO3D_LINK_TARGETS} OUTPUT_NAME Urho3D)
    set (INSTALL_TARGET Urho3D_M)
endif ()

install(TARGETS ${INSTALL_TARGET}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
)
