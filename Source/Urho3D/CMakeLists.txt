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

if (URHO3D_OPENGL)
    file (GLOB SOURCE_FILES_GRAPHICS Graphics/OpenGL/*.h Graphics/OpenGL/*.cpp)
    install (
        DIRECTORY Graphics/OpenGL
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
elseif (URHO3D_D3D11)
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

if (URHO3D_SYSTEMUI)
    file (GLOB_RECURSE SOURCE_FILES_SYSTEMUI SystemUI/*.cpp SystemUI/*.h)
    install (
        DIRECTORY SystemUI
        DESTINATION ${DEST_INCLUDE_DIR} FILES_MATCHING PATTERN *.h
    )
endif ()

# Group source code in VS solution
group_sources("Audio")
group_sources("Container")
group_sources("Core")
group_sources("Engine")
group_sources("Graphics")
group_sources("IK")
group_sources("Input")
group_sources("IO")
group_sources("Math")
group_sources("Navigation")
group_sources("Network")
group_sources("Physics")
group_sources("Resource")
group_sources("Scene")
group_sources("SystemUI")
group_sources("UI")
group_sources("Urho2D")

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
    ${SOURCE_FILES_SYSTEMUI}
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
    execute_process (COMMAND ${CMAKE_COMMAND} -DFILENAME=${CMAKE_CURRENT_BINARY_DIR}/librevision.h -P ${Urho3D_SOURCE_DIR}/CMake/Modules/GetUrhoRevision.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_QUIET ERROR_QUIET)
endif ()
target_include_directories(Urho3D PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Link dependencies
target_link_libraries(Urho3D FreeType JO LZ4 PugiXml rapidjson SDL StanHull STB Mustache IconFontCppHeaders)

if (URHO3D_NETWORK)
    target_link_libraries(Urho3D Civetweb kNet)
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

if (URHO3D_SYSTEMUI)
    target_link_libraries(Urho3D imgui ImGuizmo)
endif ()

if (URHO3D_SSE AND NOT MSVC)
    target_compile_options (Urho3D PUBLIC -msse -msse2)
endif ()

if (WIN32)
    target_link_libraries(Urho3D LibCpuId)
    target_link_libraries(Urho3D winmm user32 gdi32 winmm imm32 ole32 oleaut32 version uuid)
    if (URHO3D_MINIDUMPS)
        target_link_libraries (Urho3D dbghelp)
    endif ()
elseif (APPLE)
    target_link_libraries (Urho3D iconv "-framework AudioToolbox" "-framework CoreAudio" )
    if (ARM)
        target_link_libraries (Urho3D "-framework AVFoundation" "-framework CoreGraphics" "-framework CoreMotion" "-framework Foundation" "-framework GameController" "-framework OpenGLES" "-framework QuartzCore" "-framework UIKit")
    else ()
        target_link_libraries (Urho3D "-framework Carbon" "-framework Cocoa" "-framework CoreFoundation" "-framework SystemConfiguration" "-framework CoreServices" "-framework CoreVideo" "-framework ForceFeedback" "-framework IOKit" "-framework OpenGL")
    endif ()
elseif (ANDROID)
    target_link_libraries (Urho3D dl log android)
elseif (NOT ANDROID AND NOT ARM AND NOT WEB)
    target_link_libraries (Urho3D dl m rt)
endif ()

# Disable very annoying warning
if (GNU OR Clang)
    target_compile_options(Urho3D PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-Wno-invalid-offsetof>)
endif ()

# Graphics
if (URHO3D_OPENGL)
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
else ()
    if (URHO3D_D3D9)
        target_link_libraries(Urho3D MojoShader)
        find_package(DirectX REQUIRED D3D9)
    else ()
        find_package(DirectX REQUIRED D3D11)
    endif ()
    target_link_libraries (Urho3D ${DIRECT3D_LIBRARIES})
endif ()

if (URHO3D_TOOLS)
    target_link_libraries (Urho3D tinyfiledialogs)
endif ()

target_include_directories(Urho3D PUBLIC ${CMAKE_BINARY_DIR}/${DEST_THIRDPARTY_HEADERS})

if (BUILD_SHARED_LIBS)
    set (INSTALL_TARGET Urho3D)
    target_compile_options(Urho3D PRIVATE -DURHO3D_EXPORTS)
    if (WIN32)
        set_target_properties(Urho3D PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${DEST_BIN_DIR}")
    endif ()
else ()
    target_compile_definitions (Urho3D PUBLIC -DURHO3D_STATIC_DEFINE)
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

if (WIN32)
    set (SHARED_LIB_INSTALL_DIR bin)
else ()
    set (SHARED_LIB_INSTALL_DIR lib${LIB_SUFFIX})
endif ()

install(TARGETS ${INSTALL_TARGET}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION ${SHARED_LIB_INSTALL_DIR}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
)

# Generate platform specific pkg-config file for the benefit of Urho3D library users via SDK without CMake
get_property(URHO3D_LIBS TARGET Urho3D PROPERTY INTERFACE_LINK_LIBRARIES)
foreach (lib ${URHO3D_LIBS})
    string(REPLACE ".lib" "" cleaned_lib "${lib}")
    if (TARGET ${cleaned_lib} OR "${cleaned_lib}" STREQUAL Urho3DBase)
        list (REMOVE_ITEM URHO3D_LIBS ${lib})
    endif ()
endforeach ()
set (LIB_NAME Urho3D)
if (MSVC)
    set (DASH /)
    set (LIB_DIR "/LIBPATH:\"\${libdir}\"")
    string (REPLACE ";" ".lib\" \"" URHO3D_LIBS "\"${LIB_NAME}.lib\" \"${URHO3D_LIBS}.lib\"")
    set (PC_SUFFIX .msvc)
else ()
    set (DASH -)
    set (LIB_DIR "-L\"\${libdir}\"")
    string (REPLACE ";" " -l" URHO3D_LIBS "-l${LIB_NAME};${URHO3D_LIBS}")
endif ()
get_property(URHO3D_COMPILE_DEFINITIONS TARGET Urho3D PROPERTY INTERFACE_COMPILE_DEFINITIONS)
string (REPLACE ";" " ${DASH}D" URHO3D_COMPILE_DEFINITIONS "${DASH}D${URHO3D_COMPILE_DEFINITIONS}")
get_directory_property (GLOBAL_INCLUDE_DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} INCLUDE_DIRECTORIES)
if (GLOBAL_INCLUDE_DIRS)
    string (REPLACE ";" "\" ${DASH}I\"" GLOBAL_INCLUDE_DIRS "${DASH}I\"${GLOBAL_INCLUDE_DIRS}\"")
    string (REPLACE "${SYSROOT}" "" GLOBAL_INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS})
endif ()
set (ENGINE_INCLUDE_DIRS "${DASH}I\"\${includedir}\" ${DASH}I\"\${includedir}/Urho3D/ThirdParty\"")
if (URHO3D_PHYSICS)
    # Bullet library depends on its own include dir to be added in the header search path
    # This is more practical than patching its header files in many places to make them work with relative path
    set (ENGINE_INCLUDE_DIRS "${ENGINE_INCLUDE_DIRS} ${DASH}I\"\${includedir}/Urho3D/ThirdParty/Bullet\"")
endif ()
string (REGEX REPLACE " -include \"[^\"]+\"" "" CLEANED_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_CXX_FLAGS "${CLEANED_CMAKE_CXX_FLAGS}")
string (REPLACE "${SYSROOT}" "\${pc_sysrootdir}" CLEANED_CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
get_property (URHO3D_INTERFACE_COMPILE_OPTIONS TARGET Urho3D PROPERTY INTERFACE_COMPILE_OPTIONS)
string (REPLACE ";" " " URHO3D_INTERFACE_COMPILE_OPTIONS "${URHO3D_INTERFACE_COMPILE_OPTIONS}")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Urho3D.pc.in ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.pc${PC_SUFFIX} @ONLY)
if (MSVC)
    add_custom_command (TARGET Urho3D POST_BUILD COMMAND ${CMAKE_COMMAND} -DLIB_NAME=$<TARGET_LINKER_FILE_NAME:Urho3D> -P ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Modules/AdjustPkgConfigForMSVC.cmake)
endif ()
if (ANDROID)
    set (RENAME RENAME Urho3D-${ANDROID_NDK_ABI_NAME}.pc)
endif ()
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/Urho3D.pc DESTINATION ${DEST_PKGCONFIG_DIR} ${RENAME})
